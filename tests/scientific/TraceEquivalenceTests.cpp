#include <gtest/gtest.h>

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QStringList>

#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/nodes/SoGroup.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <cmath>
#include <cstdint>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

#include "core/CorePluginRegistry.h"
#include "core/NativeTraceSignature.h"
#include "core/RayTraceExecutor.h"
#include "core/SimulationSession.h"
#include "core/SceneInstanceBuilder.h"
#include "core/SceneLoader.h"
#include "core/TonatiuhCore.h"
#include "core/TracePreparation.h"
#include "run/GuiTraceSeed.h"
#include "kernel/air/AirExponential.h"
#include "kernel/air/AirTransmission.h"
#include "kernel/air/AirVacuum.h"
#include "kernel/photons/PhotonsAbstract.h"
#include "kernel/photons/PhotonsBuffer.h"
#include "kernel/run/InstanceNode.h"
#include "kernel/run/RayTracer.h"
#include "kernel/scene/TSceneKit.h"
#include "kernel/scene/TSeparatorKit.h"
#include "kernel/scene/TShapeKit.h"
#include "kernel/sun/SunAperture.h"
#include "kernel/sun/SunKit.h"
#include "kernel/sun/SunPosition.h"
#include "kernel/trackers/TrackerKit.h"
#include "kernel/trackers/TrackerTarget.h"
#include "libraries/math/3D/Transform.h"

namespace
{
constexpr std::uint64_t kMasterSeed = 123456789ULL;
constexpr int kSunGridDivisions = 200;
constexpr int kAxisBins = 16;
constexpr std::size_t kHistogramSize = 2 * kAxisBins * kAxisBins * kAxisBins;

enum class PreparationPath
{
    GuiBorrowed,
    HeadlessOwned
};

// Characterize the GUI scene-load lifecycle against the legacy and opt-in
// headless policies without altering production optics or random streams.
enum class TrackerScenePreparation
{
    GuiLifecycle,
    AsLoaded,
    HeadlessOptIn
};

enum class ScientificScene
{
    CylinderVacuum,
    FresnelTwoSurfaceVacuum,
    FresnelTwoSurfaceExponentialAir,
    SpecularTwoReflections,
    TrackerOneAxis
};

// Count uses of the real AirExponential model without changing its physics
// or the random-number stream.
class CountingExponentialAir final : public AirExponential
{
public:
    double transmission(double distance) const override
    {
        m_calls.fetch_add(1, std::memory_order_relaxed);
        return AirExponential::transmission(distance);
    }

    std::uint64_t calls() const
    {
        return m_calls.load(std::memory_order_relaxed);
    }

private:
    mutable std::atomic<std::uint64_t> m_calls{0};
};

// Exact callback-event multiset. Sorting after a trace removes thread-order
// dependence; raw double bits catch within-bin changes and distinguish -0.
// The comparison is within one platform, not across OS/compiler toolchains.
struct HitEvent
{
    std::string surfaceUrl;
    bool isFront = false;
    std::array<std::uint64_t, 3> positionBits{};

    bool operator==(const HitEvent& other) const
    {
        return std::tie(surfaceUrl, isFront, positionBits)
            == std::tie(other.surfaceUrl, other.isFront, other.positionBits);
    }

    bool operator<(const HitEvent& other) const
    {
        return std::tie(surfaceUrl, isFront, positionBits)
            < std::tie(other.surfaceUrl, other.isFront, other.positionBits);
    }
};

struct ScientificSignature
{
    RayTraceExecutorResult result;
    std::array<std::uint64_t, kHistogramSize> hitBins{};
    std::vector<HitEvent> hitEvents;
    std::uint64_t hitCount = 0;
    std::uint64_t frontHitCount = 0;
    std::uint64_t invalidHitCount = 0;
    std::uint64_t recordedPhotonCount = 0;
    std::uint64_t recordedTwoReflectionReceiverPaths = 0;
    std::string recordedPhotonPreview;
    std::uint64_t airTransmissionCalls = 0;
    std::uint64_t shapeInstanceCount = 0;
};

// Characterize observable propagation decisions without changing RayTracer.
// Histories retain live InstanceNode pointers, so capture surface URLs before
// the LoadedScene goes out of scope. Diagnostic tests use <= one 10k-ray chunk.
struct PropagationObservation
{
    struct CapturedHistory
    {
        RayTracerHistory history;
        std::vector<QString> surfaceUrls;
    };
    struct PhotonPoint
    {
        int id = 0;
        QString surfaceUrl;
        bool isFront = false;
        bool isAbsorbed = false;
    };

    void hit()
    {
        std::lock_guard<std::mutex> lock(mutex);
        callbacks.push_back('H');
    }

    void history(const RayTracerHistory& value)
    {
        CapturedHistory captured;
        captured.history = value;
        for (const RayTracerHistoryStep& step : value.steps)
            captured.surfaceUrls.push_back(step.surface ? step.surface->getURL() : QString());
        std::lock_guard<std::mutex> lock(mutex);
        callbacks.push_back('R');
        histories.push_back(std::move(captured));
    }

    std::mutex mutex;
    std::vector<char> callbacks;
    std::vector<CapturedHistory> histories;
    std::vector<PhotonPoint> photons;
};

// Quantize positions instead of comparing thread-dependent callback ordering.
// The bins intentionally characterize a scene; they are not a flux calculation.
int coordinateBin(double coordinate)
{
    if (coordinate <= -8.)
        return 0;
    if (coordinate >= 8.)
        return kAxisBins - 1;
    return std::clamp(static_cast<int>(std::floor(coordinate + 8.)), 0, kAxisBins - 1);
}

class HitAccumulator
{
public:
    void add(const RayTracerHit& hit)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        ++m_signature.hitCount;
        if (hit.isFront)
            ++m_signature.frontHitCount;

        if (!std::isfinite(hit.position.x) || !std::isfinite(hit.position.y)
            || !std::isfinite(hit.position.z)) {
            ++m_signature.invalidHitCount;
            return;
        }

        const int side = hit.isFront ? 1 : 0;
        const std::size_t index = static_cast<std::size_t>(
            ((side * kAxisBins + coordinateBin(hit.position.x)) * kAxisBins
                 + coordinateBin(hit.position.y)) * kAxisBins
                + coordinateBin(hit.position.z));
        ++m_signature.hitBins[index];
        m_signature.hitEvents.push_back(HitEvent{
            hit.surface ? hit.surface->getURL().toStdString() : std::string(),
            hit.isFront,
            {std::bit_cast<std::uint64_t>(hit.position.x),
             std::bit_cast<std::uint64_t>(hit.position.y),
             std::bit_cast<std::uint64_t>(hit.position.z)}
        });
    }

    void copyTo(ScientificSignature* destination) const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        destination->hitCount = m_signature.hitCount;
        destination->frontHitCount = m_signature.frontHitCount;
        destination->invalidHitCount = m_signature.invalidHitCount;
        destination->hitBins = m_signature.hitBins;
        destination->hitEvents = m_signature.hitEvents;
        std::sort(destination->hitEvents.begin(), destination->hitEvents.end());
    }

private:
    mutable std::mutex m_mutex;
    ScientificSignature m_signature;
};

QString fixturePath()
{
    return QString::fromUtf8(TONATIUHPP_EQUIVALENCE_SCENE_FILE);
}

QString fixturePath(ScientificScene scene)
{
    switch (scene) {
    case ScientificScene::CylinderVacuum:
        return fixturePath();
    case ScientificScene::FresnelTwoSurfaceVacuum:
    case ScientificScene::FresnelTwoSurfaceExponentialAir:
        return QString::fromUtf8(TONATIUHPP_FRESNEL_SCENE_FILE);
    case ScientificScene::SpecularTwoReflections:
        return QString::fromUtf8(TONATIUHPP_SPECULAR_SCENE_FILE);
    case ScientificScene::TrackerOneAxis:
        return QString::fromUtf8(TONATIUHPP_TRACKER_SCENE_FILE);
    }
    return {};
}

std::uint64_t hitsOnSurface(const ScientificSignature& signature, const std::string& path)
{
    return static_cast<std::uint64_t>(std::count_if(
        signature.hitEvents.begin(), signature.hitEvents.end(),
        [&path](const HitEvent& event) { return event.surfaceUrl == path; }));
}

std::uint64_t countShapeInstances(const InstanceNode* node)
{
    if (!node)
        return 0;
    std::uint64_t count = dynamic_cast<TShapeKit*>(node->getNode()) ? 1 : 0;
    for (const InstanceNode* child : node->children)
        count += countShapeInstances(child);
    return count;
}

// Exercise the production GUI borrowed-tree preparation without widgets.
// The preparation itself must size the sun from analytical bounds; tests must
// not pre-size it or they could mask a regression in the GUI path.
// This is not an end-to-end MainWindow automation test.
InstanceNode* findInstanceByUrl(InstanceNode* instance, const QString& url)
{
    if (!instance)
        return nullptr;
    if (instance->getURL() == url)
        return instance;
    for (InstanceNode* child : instance->children) {
        if (InstanceNode* found = findInstanceByUrl(child, url))
            return found;
    }
    return nullptr;
}

bool traceOnce(PreparationPath path, ulong rays, bool recordPhotons,
               ScientificSignature* signature, std::string* errorText,
               ScientificScene scene = ScientificScene::CylinderVacuum,
               std::uint64_t masterSeed = kMasterSeed,
               TrackerScenePreparation trackerPreparation = TrackerScenePreparation::GuiLifecycle,
               const QString& exportSurfaceUrl = QString(),
               PropagationObservation* observation = nullptr)
{
    auto fail = [errorText](const QString& message) {
        if (errorText)
            *errorText = message.toStdString();
        return false;
    };
    if (!signature)
        return fail("Missing result storage.");
    if (recordPhotons && path != PreparationPath::GuiBorrowed)
        return fail("Headless preparation does not currently accept a photon buffer.");
    if (trackerPreparation == TrackerScenePreparation::HeadlessOptIn
        && path != PreparationPath::HeadlessOwned)
        return fail("The headless tracker option requires headless preparation.");

    LoadedScene loaded;
    QString error;
    if (!SceneLoader::readFile(fixturePath(scene), &loaded, &error))
        return fail(error);

    // Native GUI scene updates drive trackers. Characterize the corresponding
    // fully prepared scene here, in both the GUI-style and headless paths.
    // TracePreparation does not currently update trackers automatically.
    if (scene == ScientificScene::TrackerOneAxis
        && trackerPreparation == TrackerScenePreparation::GuiLifecycle) {
        loaded.get()->updateParents();
        loaded.get()->updateTrackers();
    }

    // The multi-surface Fresnel scene continues rays after the first hit,
    // allowing the RayTracer's atmospheric attenuation branch to run.
    CountingExponentialAir* countedAir = nullptr;
    if (scene == ScientificScene::FresnelTwoSurfaceExponentialAir) {
        countedAir = new CountingExponentialAir;
        countedAir->constant.setValue(0.35);
        if (!loaded.get()->setPart("world.air.transmission", countedAir))
            return fail("Cannot set non-vacuum air transmission.");
    }

    SceneInstanceTree topology = SceneInstanceBuilder::build(loaded.get());
    signature->shapeInstanceCount = countShapeInstances(topology.layoutRoot);

    HitAccumulator hits;
    SceneInstanceTree borrowedTree;
    InstanceNode borrowedSun(nullptr);
    std::unique_ptr<PhotonsBuffer> photonBuffer;
    if (recordPhotons)
        photonBuffer = std::make_unique<PhotonsBuffer>(0, 0);

    PreparedTraceContext context;
    if (path == PreparationPath::GuiBorrowed) {
        borrowedTree = SceneInstanceBuilder::build(loaded.get());
        if (!borrowedTree.layoutRoot)
            return fail("GUI-style instance tree has no layout.");

        auto* sun = static_cast<SunKit*>(loaded.get()->getPart("world.sun", false));
        if (!sun)
            return fail("GUI-style scene has no sun.");
        auto* air = static_cast<AirTransmission*>(
            loaded.get()->getPart("world.air.transmission", false));
        GuiTracePreparationInput input;
        input.scene = loaded.get();
        input.layoutRoot = borrowedTree.layoutRoot;
        input.sunInstance = &borrowedSun;
        input.configuration.masterSeed = masterSeed;
        input.photonBuffer = photonBuffer.get();
        if (!exportSurfaceUrl.isEmpty()) {
            InstanceNode* selected = findInstanceByUrl(borrowedTree.layoutRoot, exportSurfaceUrl);
            if (!selected)
                return fail(QString("Unknown test export surface: %1").arg(exportSurfaceUrl));
            input.exportSurfaceList.push_back(selected);
        }
        // Use the real production policy, not a duplicated test-only condition.
        input.tracingAir = TracePreparation::effectiveTracingAir(air);
        input.hitCallback = [&hits, observation](const RayTracerHit& hit) {
            hits.add(hit);
            if (observation)
                observation->hit();
        };
        if (observation) {
            input.historyCallback = [observation](const RayTracerHistory& history) {
                observation->history(history);
            };
        }
        input.configuration.rays = rays;
        input.configuration.sunWidthDivisions = kSunGridDivisions;
        input.configuration.sunHeightDivisions = kSunGridDivisions;
        if (!TracePreparation::prepareGuiTrace(input, &context, &error))
            return fail(error);
    } else {
        HeadlessTracePreparationInput input;
        input.scene = loaded.get();
        input.updateTrackers =
            trackerPreparation == TrackerScenePreparation::HeadlessOptIn;
        input.hitCallback = [&hits, observation](const RayTracerHit& hit) {
            hits.add(hit);
            if (observation)
                observation->hit();
        };
        if (observation) {
            input.historyCallback = [observation](const RayTracerHistory& history) {
                observation->history(history);
            };
        }
        input.configuration.rays = rays;
        input.configuration.masterSeed = masterSeed;
        input.configuration.sunWidthDivisions = kSunGridDivisions;
        input.configuration.sunHeightDivisions = kSunGridDivisions;
        if (!TracePreparation::prepareHeadlessTrace(input, &context, &error))
            return fail(error);
    }

    TracePreparation::initializeResult(context, &signature->result);
    RayTraceExecutor executor;
    RayTraceExecution execution = executor.start(std::move(context));
    if (!execution.started)
        return fail(execution.errorMessage);
    if (!executor.waitForFinished(&execution, &error))
        return fail(error);
    if (!TracePreparation::finalizeResult(*execution.context(), executor.exportFailed(),
                                          0., &signature->result, &error))
        return fail(error);

    hits.copyTo(signature);
    if (photonBuffer) {
        const std::vector<Photon>& photons = photonBuffer->getPhotons();
        signature->recordedPhotonCount = photons.size();
        if (observation) {
            for (const Photon& photon : photons) {
                observation->photons.push_back(PropagationObservation::PhotonPoint{
                    photon.id,
                    photon.surface ? photon.surface->getURL() : QString(),
                    photon.isFront,
                    photon.isAbsorbed
                });
            }
        }
        if (scene == ScientificScene::SpecularTwoReflections) {
            std::ostringstream preview;
            const std::size_t previewCount = std::min<std::size_t>(photons.size(), 40);
            for (std::size_t n = 0; n < previewCount; ++n) {
                const Photon& photon = photons[n];
                preview << " " << photon.id << ":"
                        << (photon.surface
                            ? photon.surface->getURL().toStdString() : "air");
            }
            signature->recordedPhotonPreview = preview.str();
            // Photon ids restart at zero for each launched ray. A 0,1,2,3
            // run proves a single ray went Sun -> First -> Second -> Receiver,
            // unlike a global hit count that can be lower than rays launched.
            for (std::size_t i = 0; i + 3 < photons.size(); ++i) {
                const Photon& first = photons[i + 1];
                const Photon& second = photons[i + 2];
                const Photon& receiver = photons[i + 3];
                if (photons[i].id == 0
                    && first.id == 1 && first.surface
                    && first.surface->getURL() == QStringLiteral("//Node/First/Shape")
                    && second.id == 2 && second.surface
                    && second.surface->getURL() == QStringLiteral("//Node/Second/Shape")
                    && receiver.id == 3 && receiver.surface
                    && receiver.surface->getURL() == QStringLiteral("//Node/Receiver/Shape")) {
                    ++signature->recordedTwoReflectionReceiverPaths;
                }
            }
        }
    }
    if (countedAir)
        signature->airTransmissionCalls = countedAir->calls();
    return true;
}

void expectEqualScience(const ScientificSignature& expected,
                        const ScientificSignature& actual)
{
    EXPECT_EQ(expected.result.raysTraced, actual.result.raysTraced);
    EXPECT_EQ(expected.result.workerCount, actual.result.workerCount);
    EXPECT_EQ(expected.result.chunkCount, actual.result.chunkCount);
    EXPECT_EQ(expected.result.chunkSize, actual.result.chunkSize);
    EXPECT_DOUBLE_EQ(expected.result.sunApertureArea, actual.result.sunApertureArea);
    EXPECT_DOUBLE_EQ(expected.result.irradiance, actual.result.irradiance);
    EXPECT_DOUBLE_EQ(expected.result.powerPerRay, actual.result.powerPerRay);
    EXPECT_EQ(expected.result.exportFailed, actual.result.exportFailed);
    EXPECT_EQ(expected.hitCount, actual.hitCount);
    EXPECT_EQ(expected.frontHitCount, actual.frontHitCount);
    EXPECT_EQ(expected.invalidHitCount, actual.invalidHitCount);
    EXPECT_EQ(expected.shapeInstanceCount, actual.shapeInstanceCount);
    EXPECT_EQ(expected.airTransmissionCalls, actual.airTransmissionCalls);

    std::size_t differentBins = 0;
    for (std::size_t index = 0; index < kHistogramSize; ++index) {
        if (expected.hitBins[index] == actual.hitBins[index])
            continue;
        if (differentBins < 5) {
            ADD_FAILURE() << "Hit histogram bin " << index << " differs: "
                          << expected.hitBins[index] << " vs " << actual.hitBins[index];
        }
        ++differentBins;
    }
    EXPECT_EQ(differentBins, 0U);

    ASSERT_EQ(expected.hitEvents.size(), actual.hitEvents.size());
    std::size_t differentEvents = 0;
    for (std::size_t index = 0; index < expected.hitEvents.size(); ++index) {
        if (expected.hitEvents[index] == actual.hitEvents[index])
            continue;
        if (differentEvents < 3) {
            const HitEvent& a = expected.hitEvents[index];
            const HitEvent& b = actual.hitEvents[index];
            ADD_FAILURE() << "Exact hit event " << index << " differs: "
                          << a.surfaceUrl << " (front=" << a.isFront << ") at "
                          << a.positionBits[0] << "," << a.positionBits[1] << ","
                          << a.positionBits[2] << " vs "
                          << b.surfaceUrl << " (front=" << b.isFront << ") at "
                          << b.positionBits[0] << "," << b.positionBits[1] << ","
                          << b.positionBits[2];
        }
        ++differentEvents;
    }
    EXPECT_EQ(differentEvents, 0U);
}

void expectValidTrace(const ScientificSignature& signature, ulong rays)
{
    EXPECT_EQ(signature.result.raysTraced, rays);
    EXPECT_EQ(signature.result.chunkCount,
              static_cast<qulonglong>(rays / 10000 + (rays % 10000 != 0)));
    EXPECT_EQ(signature.result.chunkSize, std::min<ulong>(rays, 10000UL));
    EXPECT_FALSE(signature.result.exportFailed);
    EXPECT_EQ(signature.invalidHitCount, 0U);
}

std::array<std::uint64_t, 3> vectorBits(const vec3d& vector)
{
    return {std::bit_cast<std::uint64_t>(vector.x),
            std::bit_cast<std::uint64_t>(vector.y),
            std::bit_cast<std::uint64_t>(vector.z)};
}

// Compare full per-ray geometry, termination and surface identity. Do not
// compare raw InstanceNode pointers from independently loaded scenes.
void expectSamePropagation(const PropagationObservation& expected,
                           const PropagationObservation& actual)
{
    ASSERT_EQ(expected.histories.size(), actual.histories.size());
    for (std::size_t i = 0; i < expected.histories.size(); ++i) {
        SCOPED_TRACE(i);
        const auto& first = expected.histories[i];
        const auto& second = actual.histories[i];
        EXPECT_EQ(first.history.rayIndex, second.history.rayIndex);
        EXPECT_EQ(first.history.termination, second.history.termination);
        EXPECT_EQ(vectorBits(first.history.sunOrigin), vectorBits(second.history.sunOrigin));
        EXPECT_EQ(vectorBits(first.history.sunDirection), vectorBits(second.history.sunDirection));
        EXPECT_EQ(vectorBits(first.history.terminalOrigin), vectorBits(second.history.terminalOrigin));
        EXPECT_EQ(vectorBits(first.history.terminalDirection), vectorBits(second.history.terminalDirection));
        EXPECT_EQ(std::bit_cast<std::uint64_t>(first.history.terminalDistance),
                  std::bit_cast<std::uint64_t>(second.history.terminalDistance));
        EXPECT_EQ(first.surfaceUrls, second.surfaceUrls);
        ASSERT_EQ(first.history.steps.size(), second.history.steps.size());
        for (std::size_t j = 0; j < first.history.steps.size(); ++j) {
            const auto& a = first.history.steps[j];
            const auto& b = second.history.steps[j];
            EXPECT_EQ(a.continued, b.continued);
            EXPECT_EQ(a.isFront, b.isFront);
            EXPECT_EQ(vectorBits(a.position), vectorBits(b.position));
            EXPECT_EQ(vectorBits(a.incomingOrigin), vectorBits(b.incomingOrigin));
            EXPECT_EQ(vectorBits(a.incomingDirection), vectorBits(b.incomingDirection));
            EXPECT_EQ(vectorBits(a.outgoingOrigin), vectorBits(b.outgoingOrigin));
            EXPECT_EQ(vectorBits(a.outgoingDirection), vectorBits(b.outgoingDirection));
        }
    }
}

// One chunk, hence one worker: hit and per-ray history callbacks cannot
// interleave with callbacks from another ray. This is the CURRENT ordering,
// not an architectural requirement to retain the ordering after M2.2.
void expectLegacyCallbackOrder(const PropagationObservation& observation, bool recording)
{
    std::vector<char> expected;
    for (std::size_t i = 0; i < observation.histories.size(); ++i) {
        const RayTracerHistory& history = observation.histories[i].history;
        ASSERT_EQ(history.rayIndex, i);
        for (const RayTracerHistoryStep& step : history.steps) {
            if (step.continued)
                expected.push_back('H');
        }
        const bool terminalHit = history.termination == RayTracerHistoryTermination::Surface;
        if (!recording && terminalHit)
            expected.push_back('H');
        expected.push_back('R');
        if (recording && terminalHit)
            expected.push_back('H');
    }
    EXPECT_EQ(observation.callbacks, expected);
}

} // namespace

namespace
{
HeadlessTracePreparationInput lifecycleInput(TSceneKit* scene, ulong rays = 20001UL)
{
    HeadlessTracePreparationInput input;
    input.scene = scene;
    input.configuration.rays = rays;
    input.configuration.masterSeed = kMasterSeed;
    return input;
}

class FailingScientificExporter final : public PhotonsAbstract
{
public:
    ulong savePhotons(const std::vector<Photon>&) override { return 0; }
    bool hasExportError() const override { return true; }
};
}

TEST(ScientificSessionLifecycle, RepeatsAndRejectsConcurrentOperations)
{
    LoadedScene scene;
    QString error;
    ASSERT_TRUE(SceneLoader::readFile(fixturePath(), &scene, &error)) << error.toStdString();

    SimulationSession session;
    HitAccumulator firstHits;
    auto input = lifecycleInput(scene.get());
    input.hitCallback = [&firstHits](const RayTracerHit& hit) { firstHits.add(hit); };
    EXPECT_EQ(session.state(), SimulationSession::State::Idle);
    ASSERT_TRUE(session.prepareHeadless(input, &error)) << error.toStdString();
    EXPECT_EQ(session.state(), SimulationSession::State::Prepared);
    ASSERT_NE(session.preparedContext(), nullptr);
    ASSERT_TRUE(session.start(&error)) << error.toStdString();
    EXPECT_EQ(session.state(), SimulationSession::State::Running);
    EXPECT_FALSE(session.start(&error));
    EXPECT_FALSE(session.prepareHeadless(input, &error));
    EXPECT_EQ(session.state(), SimulationSession::State::Running);
    ASSERT_TRUE(session.waitForFinished(&error)) << error.toStdString();
    EXPECT_EQ(session.state(), SimulationSession::State::Completed);
    EXPECT_EQ(session.fullyCompletedRays(), 20001U);
    EXPECT_EQ(session.fullyCompletedChunks(), 3U);
    EXPECT_EQ(session.result().raysTraced, 20001UL);
    EXPECT_FALSE(session.result().exportFailed);

    ScientificSignature first;
    firstHits.copyTo(&first);
    HitAccumulator secondHits;
    input.hitCallback = [&secondHits](const RayTracerHit& hit) { secondHits.add(hit); };
    ASSERT_TRUE(session.prepareHeadless(input, &error)) << error.toStdString();
    ASSERT_TRUE(session.start(&error)) << error.toStdString();
    ASSERT_TRUE(session.waitForFinished(&error)) << error.toStdString();
    ScientificSignature second;
    secondHits.copyTo(&second);
    EXPECT_EQ(first.hitEvents, second.hitEvents);
    EXPECT_EQ(session.result().raysTraced, 20001UL);
    EXPECT_FALSE(session.waitForFinished(&error));
    EXPECT_EQ(session.state(), SimulationSession::State::Completed);
}

TEST(ScientificSessionLifecycle, CancellationNeverBecomesCompleted)
{
    LoadedScene scene;
    QString error;
    ASSERT_TRUE(SceneLoader::readFile(fixturePath(), &scene, &error)) << error.toStdString();

    SimulationSession session;
    auto input = lifecycleInput(scene.get(), 3000000UL);
    ASSERT_TRUE(session.prepareHeadless(input, &error)) << error.toStdString();
    ASSERT_TRUE(session.start(&error)) << error.toStdString();
    session.cancel();
    EXPECT_FALSE(session.waitForFinished(&error));
    EXPECT_EQ(session.state(), SimulationSession::State::Cancelled);
    EXPECT_EQ(session.result().raysTraced, session.fullyCompletedRays());
    EXPECT_LE(session.fullyCompletedRays(), 3000000U);
    EXPECT_FALSE(session.result().exportFailed);
}

TEST(ScientificSessionLifecycle, WorkerExceptionReportsFailure)
{
    LoadedScene scene;
    QString error;
    ASSERT_TRUE(SceneLoader::readFile(fixturePath(), &scene, &error)) << error.toStdString();

    SimulationSession session;
    auto input = lifecycleInput(scene.get(), 1024UL);
    input.hitCallback = [](const RayTracerHit&) {
        throw std::runtime_error("injected M3.1 worker failure");
    };
    ASSERT_TRUE(session.prepareHeadless(input, &error)) << error.toStdString();
    ASSERT_TRUE(session.start(&error)) << error.toStdString();
    EXPECT_FALSE(session.waitForFinished(&error));
    EXPECT_EQ(session.state(), SimulationSession::State::Failed);
    EXPECT_TRUE(error.contains("injected M3.1 worker failure")) << error.toStdString();
    EXPECT_LT(session.fullyCompletedRays(), 1024U);
}

TEST(ScientificSessionLifecycle, ExportFailureReportsIncompleteRun)
{
    LoadedScene scene;
    QString error;
    ASSERT_TRUE(SceneLoader::readFile(fixturePath(), &scene, &error)) << error.toStdString();

    FailingScientificExporter exporter;
    PhotonsBuffer photonBuffer(1);
    ASSERT_TRUE(photonBuffer.setExporter(&exporter));
    SimulationSession session;
    auto input = lifecycleInput(scene.get(), 1024UL);
    input.photonBuffer = &photonBuffer;
    ASSERT_TRUE(session.prepareHeadless(input, &error)) << error.toStdString();
    ASSERT_TRUE(session.start(&error)) << error.toStdString();
    EXPECT_FALSE(session.waitForFinished(&error));
    EXPECT_EQ(session.state(), SimulationSession::State::Failed);
    EXPECT_TRUE(session.exportFailed());
    EXPECT_TRUE(session.result().exportFailed);
    EXPECT_LE(session.fullyCompletedRays(), 1024U);
}

TEST(ScientificSessionLifecycle, DestructorJoinsWorkersAndReleasesPins)
{
    LoadedScene scene;
    QString error;
    ASSERT_TRUE(SceneLoader::readFile(fixturePath(), &scene, &error)) << error.toStdString();

    std::weak_ptr<int> weakToken;
    {
        SimulationSession session;
        auto token = std::make_shared<int>(42);
        weakToken = token;
        session.retainResource(token);
        token.reset();
        ASSERT_FALSE(weakToken.expired());
        const auto input = lifecycleInput(scene.get(), 3000000UL);
        ASSERT_TRUE(session.prepareHeadless(input, &error)) << error.toStdString();
        ASSERT_TRUE(session.start(&error)) << error.toStdString();
        // A live session destructor must cancel and join before returning.
    }
    EXPECT_TRUE(weakToken.expired());
}

TEST(ScientificTraceBaseline, HeadlessIsRepeatableAcrossIndependentScenes)
{
    for (const ulong rays : {1024UL, 20001UL}) {
        SCOPED_TRACE(rays);
        ScientificSignature first;
        ScientificSignature second;
        std::string error;
        ASSERT_TRUE(traceOnce(PreparationPath::HeadlessOwned, rays, false, &first, &error))
            << error;
        ASSERT_TRUE(traceOnce(PreparationPath::HeadlessOwned, rays, false, &second, &error))
            << error;
        expectValidTrace(first, rays);
        expectValidTrace(second, rays);
        expectEqualScience(first, second);
    }
}

TEST(ScientificTraceBaseline, GuiStylePreparationIsRepeatableWithoutWidgets)
{
    for (const ulong rays : {1024UL, 20001UL}) {
        SCOPED_TRACE(rays);
        ScientificSignature first;
        ScientificSignature second;
        std::string error;
        ASSERT_TRUE(traceOnce(PreparationPath::GuiBorrowed, rays, false, &first, &error))
            << error;
        ASSERT_TRUE(traceOnce(PreparationPath::GuiBorrowed, rays, false, &second, &error))
            << error;
        expectValidTrace(first, rays);
        expectValidTrace(second, rays);
        expectEqualScience(first, second);
    }
}

// Regression: GUI borrowed and headless owned preparation must independently
// compute identical analytical ray-tracing sun geometry.
TEST(ScientificTraceDiagnostic, GuiStyleMatchesHeadless)
{
    for (const ulong rays : {1024UL, 20001UL}) {
        SCOPED_TRACE(rays);
        ScientificSignature gui;
        ScientificSignature headless;
        std::string error;
        ASSERT_TRUE(traceOnce(PreparationPath::GuiBorrowed, rays, false, &gui, &error))
            << error;
        ASSERT_TRUE(traceOnce(PreparationPath::HeadlessOwned, rays, false, &headless, &error))
            << error;
        expectEqualScience(gui, headless);
    }
}

// Regression: photon buffering exercises the other RayTracer propagation loop,
// without starting a file exporter. Keep checking equivalence during refactoring.
TEST(ScientificTraceDiagnostic, PhotonRecordingPreservesScientificHits)
{
    ScientificSignature withoutRecording;
    ScientificSignature withRecording;
    std::string error;
    ASSERT_TRUE(traceOnce(PreparationPath::GuiBorrowed, 1024UL, false,
                          &withoutRecording, &error)) << error;
    ASSERT_TRUE(traceOnce(PreparationPath::GuiBorrowed, 1024UL, true,
                          &withRecording, &error)) << error;
    EXPECT_GT(withRecording.recordedPhotonCount, 0U);
    expectEqualScience(withoutRecording, withRecording);
}

// M2.1: A0 already checks identical exact hits; characterize the observable
// callback ordering difference on terminal surface interactions as well.
TEST(ScientificPropagationSemantics, TerminalCallbackOrderDiffersByRecordingMode)
{
    constexpr ulong rays = 1024UL;  // exactly one worker chunk
    ScientificSignature noRecord, recorded;
    PropagationObservation noRecordEvents, recordedEvents;
    std::string error;
    ASSERT_TRUE(traceOnce(PreparationPath::GuiBorrowed, rays, false, &noRecord, &error,
                          ScientificScene::CylinderVacuum, kMasterSeed,
                          TrackerScenePreparation::GuiLifecycle, QString(), &noRecordEvents)) << error;
    ASSERT_TRUE(traceOnce(PreparationPath::GuiBorrowed, rays, true, &recorded, &error,
                          ScientificScene::CylinderVacuum, kMasterSeed,
                          TrackerScenePreparation::GuiLifecycle, QString(), &recordedEvents)) << error;
    expectEqualScience(noRecord, recorded);
    ASSERT_EQ(noRecordEvents.histories.size(), rays);
    ASSERT_EQ(recordedEvents.histories.size(), rays);
    expectSamePropagation(noRecordEvents, recordedEvents);
    expectLegacyCallbackOrder(noRecordEvents, false);
    expectLegacyCallbackOrder(recordedEvents, true);
    EXPECT_GT(std::count_if(noRecordEvents.histories.begin(), noRecordEvents.histories.end(),
                             [](const auto& event) {
                                 return event.history.termination == RayTracerHistoryTermination::Surface;
                             }), 0);
    EXPECT_GT(recorded.recordedPhotonCount, 0U);
    EXPECT_TRUE(std::any_of(recordedEvents.photons.begin(), recordedEvents.photons.end(),
                            [](const auto& photon) {
                                return photon.id == 0 && photon.isFront;
                            }));
}

// M2.1: Export-surface filtering currently affects recording-mode terminal
// hit callbacks but NOT non-recording terminal hits or intermediate hits.
// This is a measured legacy divergence; M2.2 must decide its explicit policy.
TEST(ScientificPropagationSemantics, FilteredExportSuppressesRecordedTerminalHits)
{
    constexpr ulong rays = 4096UL;
    const QString firstSurface = QStringLiteral("//Node/First/Shape");
    const std::string receiverSurface = "//Node/Receiver/Shape";
    ScientificSignature noRecord, recorded;
    PropagationObservation noRecordEvents, recordedEvents;
    std::string error;
    ASSERT_TRUE(traceOnce(PreparationPath::GuiBorrowed, rays, false, &noRecord, &error,
                          ScientificScene::SpecularTwoReflections, kMasterSeed,
                          TrackerScenePreparation::GuiLifecycle, firstSurface, &noRecordEvents)) << error;
    ASSERT_TRUE(traceOnce(PreparationPath::GuiBorrowed, rays, true, &recorded, &error,
                          ScientificScene::SpecularTwoReflections, kMasterSeed,
                          TrackerScenePreparation::GuiLifecycle, firstSurface, &recordedEvents)) << error;
    ASSERT_EQ(noRecordEvents.histories.size(), rays);
    ASSERT_EQ(recordedEvents.histories.size(), rays);
    expectSamePropagation(noRecordEvents, recordedEvents);
    EXPECT_EQ(hitsOnSurface(noRecord, "//Node/First/Shape"),
              hitsOnSurface(recorded, "//Node/First/Shape"));
    EXPECT_GT(hitsOnSurface(noRecord, receiverSurface), 0U);
    EXPECT_EQ(hitsOnSurface(recorded, receiverSurface), 0U);
    EXPECT_GT(hitsOnSurface(recorded, "//Node/Second/Shape"), 0U)
        << "Intermediate reflected hits remain visible despite export filtering.";
    ASSERT_FALSE(recordedEvents.photons.empty());
    for (const auto& photon : recordedEvents.photons) {
        EXPECT_EQ(photon.surfaceUrl, firstSurface);
        EXPECT_GT(photon.id, 0) << "Filtering must not renumber path interaction IDs.";
    }
}

// M2.1: Atmospheric loss must carry the same sampled segment distance and
// terminal geometry with or without photon recording, including escape paths.
TEST(ScientificPropagationSemantics, AirLossTerminalRecordsMatchWithRecording)
{
    constexpr ulong rays = 4096UL;
    ScientificSignature noRecord, recorded;
    PropagationObservation noRecordEvents, recordedEvents;
    std::string error;
    ASSERT_TRUE(traceOnce(PreparationPath::GuiBorrowed, rays, false, &noRecord, &error,
                          ScientificScene::FresnelTwoSurfaceExponentialAir, kMasterSeed,
                          TrackerScenePreparation::GuiLifecycle, QString(), &noRecordEvents)) << error;
    ASSERT_TRUE(traceOnce(PreparationPath::GuiBorrowed, rays, true, &recorded, &error,
                          ScientificScene::FresnelTwoSurfaceExponentialAir, kMasterSeed,
                          TrackerScenePreparation::GuiLifecycle, QString(), &recordedEvents)) << error;
    expectEqualScience(noRecord, recorded);
    ASSERT_EQ(noRecordEvents.histories.size(), rays);
    ASSERT_EQ(recordedEvents.histories.size(), rays);
    expectSamePropagation(noRecordEvents, recordedEvents);
    expectLegacyCallbackOrder(noRecordEvents, false);
    expectLegacyCallbackOrder(recordedEvents, true);
    EXPECT_GT(noRecord.airTransmissionCalls, 0U);
    EXPECT_GT(std::count_if(noRecordEvents.histories.begin(), noRecordEvents.histories.end(),
                             [](const auto& event) {
                                 return event.history.termination == RayTracerHistoryTermination::Air;
                             }), 0);
}

// This stable two-surface Fresnel fixture exercises material continuation,
// a receiver, the borrowed GUI-style contract and owned headless preparation.
TEST(ScientificTraceExtended, FresnelTwoSurfaceIsRepeatableAndMatchesPaths)
{
    for (const ulong rays : {4096UL, 20001UL}) {
        SCOPED_TRACE(rays);
        ScientificSignature first;
        ScientificSignature repeated;
        ScientificSignature gui;
        std::string error;
        ASSERT_TRUE(traceOnce(PreparationPath::HeadlessOwned, rays, false,
                              &first, &error, ScientificScene::FresnelTwoSurfaceVacuum)) << error;
        ASSERT_TRUE(traceOnce(PreparationPath::HeadlessOwned, rays, false,
                              &repeated, &error, ScientificScene::FresnelTwoSurfaceVacuum)) << error;
        ASSERT_TRUE(traceOnce(PreparationPath::GuiBorrowed, rays, false,
                              &gui, &error, ScientificScene::FresnelTwoSurfaceVacuum)) << error;

        expectValidTrace(first, rays);
        expectValidTrace(gui, rays);
        EXPECT_GE(first.shapeInstanceCount, 2U);
        EXPECT_GT(first.hitCount, 0U);
        EXPECT_EQ(first.airTransmissionCalls, 0U);
        expectEqualScience(first, repeated);
        expectEqualScience(first, gui);
    }
}

TEST(ScientificTraceExtended, ExponentialAirIsExercisedAndMatchesPaths)
{
    constexpr ulong rays = 4096UL;
    ScientificSignature first;
    ScientificSignature repeated;
    ScientificSignature gui;
    std::string error;
    ASSERT_TRUE(traceOnce(PreparationPath::HeadlessOwned, rays, false,
                          &first, &error, ScientificScene::FresnelTwoSurfaceExponentialAir)) << error;
    ASSERT_TRUE(traceOnce(PreparationPath::HeadlessOwned, rays, false,
                          &repeated, &error, ScientificScene::FresnelTwoSurfaceExponentialAir)) << error;
    ASSERT_TRUE(traceOnce(PreparationPath::GuiBorrowed, rays, false,
                          &gui, &error, ScientificScene::FresnelTwoSurfaceExponentialAir)) << error;

    expectValidTrace(first, rays);
    EXPECT_GE(first.shapeInstanceCount, 2U);
    EXPECT_GT(first.hitCount, 0U);
    EXPECT_GT(first.airTransmissionCalls, 0U)
        << "A non-vacuum regression must exercise atmospheric transmission.";
    expectEqualScience(first, repeated);
    expectEqualScience(first, gui);
}

TEST(ScientificHitEvents, ExactMultisetDistinguishesWithinBinAndSurfaceChanges)
{
    EXPECT_EQ(coordinateBin(0.25), coordinateBin(0.75));
    const HitEvent original{
        "//Node/First/Shape", true,
        {std::bit_cast<std::uint64_t>(0.25), std::bit_cast<std::uint64_t>(0.5),
         std::bit_cast<std::uint64_t>(0.)}
    };
    HitEvent moved = original;
    moved.positionBits[0] = std::bit_cast<std::uint64_t>(0.75);
    EXPECT_FALSE(original == moved);
    HitEvent otherSurface = original;
    otherSurface.surfaceUrl = "//Node/Second/Shape";
    EXPECT_FALSE(original == otherSurface);
    HitEvent otherSide = original;
    otherSide.isFront = false;
    EXPECT_FALSE(original == otherSide);
}

TEST(ScientificTraceExtended, SpecularTwoReflectionsRepeatAndMatchPreparation)
{
    for (const ulong rays : {4096UL, 20001UL}) {
        SCOPED_TRACE(rays);
        ScientificSignature headless;
        ScientificSignature repeated;
        ScientificSignature gui;
        std::string error;
        ASSERT_TRUE(traceOnce(PreparationPath::HeadlessOwned, rays, false,
                              &headless, &error, ScientificScene::SpecularTwoReflections)) << error;
        ASSERT_TRUE(traceOnce(PreparationPath::HeadlessOwned, rays, false,
                              &repeated, &error, ScientificScene::SpecularTwoReflections)) << error;
        ASSERT_TRUE(traceOnce(PreparationPath::GuiBorrowed, rays, false,
                              &gui, &error, ScientificScene::SpecularTwoReflections)) << error;

        expectValidTrace(headless, rays);
        EXPECT_EQ(headless.shapeInstanceCount, 3U);
        EXPECT_GT(hitsOnSurface(headless, "//Node/First/Shape"), 0U);
        EXPECT_GT(hitsOnSurface(headless, "//Node/Second/Shape"), 0U);
        EXPECT_GT(hitsOnSurface(headless, "//Node/Receiver/Shape"), 0U);
        // Total callback events need not exceed rays launched: some rays miss
        // or terminate early. Recording tests below verify full bounce paths.
        expectEqualScience(headless, repeated);
        expectEqualScience(headless, gui);
    }
}

TEST(ScientificTraceExtended, SpecularRecordingPreservesExactEvents)
{
    constexpr ulong rays = 4096UL;
    ScientificSignature withoutRecording;
    ScientificSignature withRecording;
    std::string error;
    ASSERT_TRUE(traceOnce(PreparationPath::GuiBorrowed, rays, false,
                          &withoutRecording, &error,
                          ScientificScene::SpecularTwoReflections)) << error;
    ASSERT_TRUE(traceOnce(PreparationPath::GuiBorrowed, rays, true,
                          &withRecording, &error,
                          ScientificScene::SpecularTwoReflections)) << error;
    EXPECT_GT(withRecording.recordedPhotonCount, 0U);
    EXPECT_GT(withRecording.recordedTwoReflectionReceiverPaths, 0U)
        << "Expected at least one complete Sun -> First -> Second -> Receiver path."
        << "\nRecorded photons: " << withRecording.recordedPhotonCount
        << "\nFirst photons (id:surface):" << withRecording.recordedPhotonPreview;
    expectEqualScience(withoutRecording, withRecording);
}

TEST(ScientificTracker, OneAxisUpdateRotatesMirrorAndRespondsToSun)
{
    LoadedScene loaded;
    QString error;
    ASSERT_TRUE(SceneLoader::readFile(fixturePath(ScientificScene::TrackerOneAxis),
                                      &loaded, &error)) << error.toStdString();
    auto* sceneGroup = static_cast<SoGroup*>(loaded.get()->getLayout()->getPart("group", false));
    ASSERT_NE(sceneGroup, nullptr);
    auto* assembly = dynamic_cast<TSeparatorKit*>(sceneGroup->getChild(0));
    ASSERT_NE(assembly, nullptr);
    auto* assemblyGroup = static_cast<SoGroup*>(assembly->getPart("group", false));
    ASSERT_NE(assemblyGroup, nullptr);
    auto* tracker = dynamic_cast<TrackerKit*>(assemblyGroup->getChild(0));
    auto* primary = dynamic_cast<TSeparatorKit*>(assemblyGroup->getChild(1));
    ASSERT_NE(tracker, nullptr);
    ASSERT_NE(primary, nullptr);
    auto* target = dynamic_cast<TrackerTarget*>(tracker->target.getValue());
    auto* primaryTransform = dynamic_cast<TTransform*>(primary->getPart("transform", false));
    auto* sun = static_cast<SunPosition*>(
        loaded.get()->getPart("world.sun.position", false));
    ASSERT_NE(target, nullptr);
    ASSERT_NE(primaryTransform, nullptr);
    ASSERT_NE(sun, nullptr);

    // The GUI scene setup initializes this backlink before tracking.
    // Without it TrackerKit updates its target angle but silently skips
    // the primary transform in onSensor_target().
    loaded.get()->updateParents();
    ASSERT_EQ(tracker->m_parent, assembly);
    loaded.get()->updateTrackers();
    const float initialAngle = target->angles.getValue()[0];
    SbVec3f axis;
    float radians = 0.f;
    primaryTransform->rotation.getValue().getValue(axis, radians);
    EXPECT_GT(std::abs(initialAngle), 1.f);
    EXPECT_GT(std::abs(radians), 0.01f);

    sun->azimuth.setValue(270.);
    loaded.get()->updateTrackers();
    const float updatedAngle = target->angles.getValue()[0];
    EXPECT_GT(std::abs(updatedAngle - initialAngle), 1.f);

    tracker->enabled = false;
    sun->azimuth.setValue(90.);
    loaded.get()->updateTrackers();
    EXPECT_FLOAT_EQ(target->angles.getValue()[0], updatedAngle);
}

TEST(ScientificTracker, UpdatedSceneRepeatsAndMatchesPreparation)
{
    constexpr ulong rays = 4096UL;
    ScientificSignature headless;
    ScientificSignature repeated;
    ScientificSignature gui;
    std::string error;
    ASSERT_TRUE(traceOnce(PreparationPath::HeadlessOwned, rays, false,
                          &headless, &error, ScientificScene::TrackerOneAxis)) << error;
    ASSERT_TRUE(traceOnce(PreparationPath::HeadlessOwned, rays, false,
                          &repeated, &error, ScientificScene::TrackerOneAxis)) << error;
    ASSERT_TRUE(traceOnce(PreparationPath::GuiBorrowed, rays, false,
                          &gui, &error, ScientificScene::TrackerOneAxis)) << error;

    expectValidTrace(headless, rays);
    EXPECT_EQ(headless.shapeInstanceCount, 2U);
    EXPECT_GT(hitsOnSurface(headless, "//Node/TrackingAssembly/Primary/Shape"), 0U);
    EXPECT_GT(hitsOnSurface(headless, "//Node/Target/Shape"), 0U);
    expectEqualScience(headless, repeated);
    expectEqualScience(headless, gui);
}

TEST(ScientificTracker, HeadlessOptInMatchesGuiLifecycleAndDiffersFromLegacy)
{
    // Same fixture and master seed, separately loaded scenes. Only the
    // tracker-preparation policy changes; check complete hit event multisets.
    constexpr ulong rays = 20001UL;
    ScientificSignature gui;
    ScientificSignature headlessOptIn;
    ScientificSignature headlessRepeated;
    ScientificSignature legacy;
    ScientificSignature legacyRepeated;
    std::string error;

    ASSERT_TRUE(traceOnce(PreparationPath::GuiBorrowed, rays, false,
                          &gui, &error, ScientificScene::TrackerOneAxis)) << error;
    ASSERT_TRUE(traceOnce(PreparationPath::HeadlessOwned, rays, false,
                          &headlessOptIn, &error, ScientificScene::TrackerOneAxis,
                          kMasterSeed, TrackerScenePreparation::HeadlessOptIn)) << error;
    ASSERT_TRUE(traceOnce(PreparationPath::HeadlessOwned, rays, false,
                          &headlessRepeated, &error, ScientificScene::TrackerOneAxis,
                          kMasterSeed, TrackerScenePreparation::HeadlessOptIn)) << error;
    ASSERT_TRUE(traceOnce(PreparationPath::HeadlessOwned, rays, false,
                          &legacy, &error, ScientificScene::TrackerOneAxis,
                          kMasterSeed, TrackerScenePreparation::AsLoaded)) << error;
    ASSERT_TRUE(traceOnce(PreparationPath::HeadlessOwned, rays, false,
                          &legacyRepeated, &error, ScientificScene::TrackerOneAxis,
                          kMasterSeed, TrackerScenePreparation::AsLoaded)) << error;

    expectValidTrace(gui, rays);
    expectValidTrace(legacy, rays);
    EXPECT_GT(hitsOnSurface(gui, "//Node/Target/Shape"), 0U);

    expectEqualScience(gui, headlessOptIn);
    expectEqualScience(headlessOptIn, headlessRepeated);

    // Do not silently migrate legacy default behavior or benchmark outputs.
    expectEqualScience(legacy, legacyRepeated);
    EXPECT_FALSE(legacy.hitEvents == gui.hitEvents)
        << "As-loaded headless unexpectedly matches the rotated GUI tracker.";
}

TEST(ScientificTracker, OptInPreservesNoTrackerScene)
{
    constexpr ulong rays = 4096UL;
    ScientificSignature legacy;
    ScientificSignature optIn;
    std::string error;
    ASSERT_TRUE(traceOnce(PreparationPath::HeadlessOwned, rays, false,
                          &legacy, &error, ScientificScene::CylinderVacuum,
                          kMasterSeed, TrackerScenePreparation::AsLoaded)) << error;
    ASSERT_TRUE(traceOnce(PreparationPath::HeadlessOwned, rays, false,
                          &optIn, &error, ScientificScene::CylinderVacuum,
                          kMasterSeed, TrackerScenePreparation::HeadlessOptIn)) << error;
    expectValidTrace(legacy, rays);
    expectEqualScience(legacy, optIn);
}

TEST(ScientificTraceExtended, ExponentialAirPhotonRecordingPreservesHits)
{
    constexpr ulong rays = 4096UL;
    ScientificSignature withoutRecording;
    ScientificSignature withRecording;
    std::string error;
    ASSERT_TRUE(traceOnce(PreparationPath::GuiBorrowed, rays, false,
                          &withoutRecording, &error,
                          ScientificScene::FresnelTwoSurfaceExponentialAir)) << error;
    ASSERT_TRUE(traceOnce(PreparationPath::GuiBorrowed, rays, true,
                          &withRecording, &error,
                          ScientificScene::FresnelTwoSurfaceExponentialAir)) << error;

    EXPECT_GT(withoutRecording.airTransmissionCalls, 0U);
    EXPECT_GT(withRecording.recordedPhotonCount, 0U);
    expectEqualScience(withoutRecording, withRecording);
}

// Compare independent optical scene bounds with a correctly applied Coin3D
// action; the exact GUI production sizing call has a separate opt-in test.
TEST(ScientificTraceExtended, InventorSceneBoundingActionTraversesFixtures)
{
    for (ScientificScene scene : {ScientificScene::CylinderVacuum,
                                  ScientificScene::FresnelTwoSurfaceVacuum}) {
        SCOPED_TRACE(fixturePath(scene).toStdString());
        LoadedScene loaded;
        QString error;
        ASSERT_TRUE(SceneLoader::readFile(fixturePath(scene), &loaded, &error))
            << error.toStdString();
        ASSERT_NE(loaded.get()->getLayout(), nullptr);

        SoGetBoundingBoxAction action{SbViewportRegion()};
        action.apply(loaded.get()->getLayout());
        const SbBox3f sceneBox = action.getBoundingBox();
        ASSERT_FALSE(sceneBox.isEmpty());
        for (int axis = 0; axis < 3; ++axis) {
            const double minimum = sceneBox.getMin()[axis];
            const double maximum = sceneBox.getMax()[axis];
            EXPECT_TRUE(std::isfinite(minimum));
            EXPECT_TRUE(std::isfinite(maximum));
            EXPECT_GE(maximum, minimum);
        }

        SceneInstanceTree tree = SceneInstanceBuilder::build(loaded.get());
        ASSERT_NE(tree.layoutRoot, nullptr);
        tree.layoutRoot->updateTree(Transform::Identity);
        EXPECT_TRUE(tree.layoutRoot->getBox().isValid());
    }
}

// M0C: Both preparation entry points must overwrite any legacy/stale sun
// sizing with the same analytical bounds before computing cells and power.
TEST(ScientificSunSizing, GuiAndHeadlessUseIdenticalAnalyticalBounds)
{
    for (ScientificScene scene : {ScientificScene::CylinderVacuum,
                                  ScientificScene::FresnelTwoSurfaceVacuum}) {
        SCOPED_TRACE(fixturePath(scene).toStdString());
        LoadedScene guiScene;
        LoadedScene headlessScene;
        QString error;
        ASSERT_TRUE(SceneLoader::readFile(fixturePath(scene), &guiScene, &error))
            << error.toStdString();
        ASSERT_TRUE(SceneLoader::readFile(fixturePath(scene), &headlessScene, &error))
            << error.toStdString();

        SceneInstanceTree borrowed = SceneInstanceBuilder::build(guiScene.get());
        ASSERT_NE(borrowed.layoutRoot, nullptr);
        InstanceNode guiSun(nullptr);
        auto* guiSunKit = static_cast<SunKit*>(guiScene.get()->getPart("world.sun", false));
        auto* headlessSunKit = static_cast<SunKit*>(headlessScene.get()->getPart("world.sun", false));
        ASSERT_NE(guiSunKit, nullptr);
        ASSERT_NE(headlessSunKit, nullptr);

        // Deliberately corrupt only the old GUI sun bounding setup; the
        // production preparation must replace it with optical scene bounds.
        guiSunKit->setBox(Box3D(vec3d(-9., -9., -9.), vec3d(9., 9., 9.)));

        constexpr ulong rays = 4096UL;
        GuiTracePreparationInput guiInput;
        guiInput.scene = guiScene.get();
        guiInput.layoutRoot = borrowed.layoutRoot;
        guiInput.sunInstance = &guiSun;
        guiInput.configuration.rays = rays;
        guiInput.configuration.masterSeed = kMasterSeed;
        guiInput.configuration.sunWidthDivisions = kSunGridDivisions;
        guiInput.configuration.sunHeightDivisions = kSunGridDivisions;
        PreparedTraceContext guiContext;
        ASSERT_TRUE(TracePreparation::prepareGuiTrace(guiInput, &guiContext, &error))
            << error.toStdString();

        HeadlessTracePreparationInput headlessInput;
        headlessInput.scene = headlessScene.get();
        headlessInput.configuration.rays = rays;
        headlessInput.configuration.masterSeed = kMasterSeed;
        headlessInput.configuration.sunWidthDivisions = kSunGridDivisions;
        headlessInput.configuration.sunHeightDivisions = kSunGridDivisions;
        PreparedTraceContext headlessContext;
        ASSERT_TRUE(TracePreparation::prepareHeadlessTrace(headlessInput, &headlessContext, &error))
            << error.toStdString();

        auto* guiAperture = static_cast<SunAperture*>(guiSunKit->getPart("aperture", false));
        auto* headlessAperture = static_cast<SunAperture*>(headlessSunKit->getPart("aperture", false));
        ASSERT_NE(guiAperture, nullptr);
        ASSERT_NE(headlessAperture, nullptr);
        EXPECT_EQ(guiAperture->getCells(), headlessAperture->getCells());
        EXPECT_GT(guiAperture->getCells().size(), 0U);
        EXPECT_DOUBLE_EQ(guiContext.sunApertureArea(), headlessContext.sunApertureArea());
        EXPECT_DOUBLE_EQ(guiContext.powerPerRay(), headlessContext.powerPerRay());
        EXPECT_GT(guiContext.powerPerRay(), 0.);
    }
}

// M1: adapters must converge on shared geometry/sun setup while preserving
// borrowed GUI ownership and owned headless preparation progress.
TEST(ScientificTracePreparation, CommonPipelinePreservesAdapterContracts)
{
    LoadedScene guiScene;
    LoadedScene headlessScene;
    QString error;
    ASSERT_TRUE(SceneLoader::readFile(fixturePath(), &guiScene, &error))
        << error.toStdString();
    ASSERT_TRUE(SceneLoader::readFile(fixturePath(), &headlessScene, &error))
        << error.toStdString();

    SceneInstanceTree borrowedTree = SceneInstanceBuilder::build(guiScene.get());
    ASSERT_NE(borrowedTree.layoutRoot, nullptr);
    InstanceNode borrowedSun(nullptr);
    int synchronizeCalls = 0;

    GuiTracePreparationInput gui;
    gui.scene = guiScene.get();
    gui.layoutRoot = borrowedTree.layoutRoot;
    gui.sunInstance = &borrowedSun;
    gui.configuration.rays = 4096;
    gui.configuration.masterSeed = kMasterSeed;
    gui.synchronizeScene = [&synchronizeCalls] { ++synchronizeCalls; };

    PreparedTraceContext guiContext;
    ASSERT_TRUE(TracePreparation::prepareGuiTrace(gui, &guiContext, &error))
        << error.toStdString();
    EXPECT_EQ(synchronizeCalls, 1);
    EXPECT_EQ(guiContext.rays(), 4096UL);
    EXPECT_EQ(guiContext.photonBuffer(), nullptr);

    HeadlessTracePreparationInput headless;
    headless.scene = headlessScene.get();
    headless.configuration = gui.configuration;
    QStringList progress;
    headless.progress = [&progress](const QString& message) { progress << message; };

    PreparedTraceContext headlessContext;
    ASSERT_TRUE(TracePreparation::prepareHeadlessTrace(headless, &headlessContext, &error))
        << error.toStdString();
    EXPECT_EQ(headlessContext.rays(), 4096UL);
    EXPECT_EQ(headlessContext.photonBuffer(), nullptr);
    EXPECT_EQ(progress, (QStringList{
        QStringLiteral("Preparing scene."),
        QStringLiteral("Building ray-tracing instance tree."),
        QStringLiteral("Updating ray-tracing instance tree."),
        QStringLiteral("Sizing sun aperture."),
        QStringLiteral("Finding sun aperture cells.")
    }));

    auto* guiAperture = static_cast<SunAperture*>(
        guiScene.get()->getPart("world.sun.aperture", false));
    auto* headlessAperture = static_cast<SunAperture*>(
        headlessScene.get()->getPart("world.sun.aperture", false));
    ASSERT_NE(guiAperture, nullptr);
    ASSERT_NE(headlessAperture, nullptr);
    EXPECT_EQ(guiAperture->getCells(), headlessAperture->getCells());
    EXPECT_DOUBLE_EQ(guiContext.sunApertureArea(), headlessContext.sunApertureArea());
    EXPECT_DOUBLE_EQ(guiContext.irradiance(), headlessContext.irradiance());
    EXPECT_DOUBLE_EQ(guiContext.powerPerRay(), headlessContext.powerPerRay());
}

// M1.3: each caller provides effective values to the same prepared-scene
// contract. Compare borrowed GUI and owned headless state before tracing,
// including a non-default grid and opt-in tracker synchronization.
TEST(ScientificTracePreparation, EffectiveInputsAndDiagnosticMetadataMatchAcrossAdapters)
{
    struct Case
    {
        ScientificScene scene;
        int width;
        int height;
    };
    const std::array<Case, 3> cases = {{{ScientificScene::CylinderVacuum, 119, 121},
                                        {ScientificScene::SpecularTwoReflections, 200, 200},
                                        {ScientificScene::TrackerOneAxis, 200, 200}}};
    for (const Case& scenario : cases) {
        const QString path = fixturePath(scenario.scene);
        SCOPED_TRACE(path.toStdString());
        LoadedScene guiScene;
        LoadedScene headlessScene;
        QString error;
        ASSERT_TRUE(SceneLoader::readFile(path, &guiScene, &error)) << error.toStdString();
        ASSERT_TRUE(SceneLoader::readFile(path, &headlessScene, &error)) << error.toStdString();

        const bool tracker = scenario.scene == ScientificScene::TrackerOneAxis;
        if (tracker) {
            // Match the native GUI scene lifecycle; headless only updates
            // trackers when the caller explicitly opts in.
            guiScene.get()->updateParents();
            guiScene.get()->updateTrackers();
        }
        SceneInstanceTree borrowed = SceneInstanceBuilder::build(guiScene.get());
        ASSERT_NE(borrowed.layoutRoot, nullptr);
        InstanceNode guiSun(nullptr);

        SimulationConfig configuration;
        configuration.rays = 20001UL;
        configuration.masterSeed = kMasterSeed;
        configuration.sunWidthDivisions = scenario.width;
        configuration.sunHeightDivisions = scenario.height;

        GuiTracePreparationInput gui;
        gui.scene = guiScene.get();
        gui.layoutRoot = borrowed.layoutRoot;
        gui.sunInstance = &guiSun;
        gui.configuration = configuration;
        PreparedTraceContext guiContext;
        ASSERT_TRUE(TracePreparation::prepareGuiTrace(gui, &guiContext, &error))
            << error.toStdString();

        HeadlessTracePreparationInput headless;
        headless.scene = headlessScene.get();
        headless.configuration = configuration;
        headless.updateTrackers = tracker;
        PreparedTraceContext headlessContext;
        ASSERT_TRUE(TracePreparation::prepareHeadlessTrace(headless, &headlessContext, &error))
            << error.toStdString();

        const auto guiInputs = NativeTraceSignature::inputsForPreparedTrace(path, guiContext);
        const auto headlessInputs = NativeTraceSignature::inputsForPreparedTrace(path, headlessContext);
        EXPECT_EQ(guiContext.rays(), configuration.rays);
        EXPECT_EQ(headlessContext.rays(), configuration.rays);
        EXPECT_EQ(guiContext.configuration().masterSeed, kMasterSeed);
        EXPECT_EQ(headlessContext.configuration().masterSeed, kMasterSeed);
        EXPECT_EQ(guiInputs.sceneFile, path);
        EXPECT_EQ(guiInputs.sceneFile, headlessInputs.sceneFile);
        EXPECT_EQ(guiInputs.rays, headlessInputs.rays);
        EXPECT_EQ(guiInputs.seed, headlessInputs.seed);
        EXPECT_EQ(guiInputs.sunGridWidth, scenario.width);
        EXPECT_EQ(guiInputs.sunGridHeight, scenario.height);
        EXPECT_EQ(guiInputs.sunGridWidth, headlessInputs.sunGridWidth);
        EXPECT_EQ(guiInputs.sunGridHeight, headlessInputs.sunGridHeight);
        EXPECT_DOUBLE_EQ(guiInputs.apertureArea, headlessInputs.apertureArea);
        EXPECT_DOUBLE_EQ(guiInputs.irradiance, headlessInputs.irradiance);
        EXPECT_DOUBLE_EQ(guiInputs.powerPerRay, headlessInputs.powerPerRay);
        EXPECT_GT(guiInputs.powerPerRay, 0.);

        auto* guiAperture = static_cast<SunAperture*>(
            guiScene.get()->getPart("world.sun.aperture", false));
        auto* headlessAperture = static_cast<SunAperture*>(
            headlessScene.get()->getPart("world.sun.aperture", false));
        ASSERT_NE(guiAperture, nullptr);
        ASSERT_NE(headlessAperture, nullptr);
        EXPECT_EQ(guiAperture->getCells(), headlessAperture->getCells());

        RayTraceExecutorResult guiResult;
        RayTraceExecutorResult headlessResult;
        TracePreparation::initializeResult(guiContext, &guiResult);
        TracePreparation::initializeResult(headlessContext, &headlessResult);
        EXPECT_EQ(guiResult.chunkCount, 3ULL);
        EXPECT_EQ(guiResult.chunkCount, headlessResult.chunkCount);
        EXPECT_EQ(guiResult.chunkSize, headlessResult.chunkSize);
        EXPECT_DOUBLE_EQ(guiResult.powerPerRay, headlessResult.powerPerRay);
    }
}

TEST(ScientificGuiSeed, ParsesPortableDecimalInput)
{
    std::uint64_t seed = 99;
    EXPECT_TRUE(GuiTraceSeed::parseFixedSeed(QStringLiteral("0"), &seed));
    EXPECT_EQ(seed, 0ULL);
    EXPECT_TRUE(GuiTraceSeed::parseFixedSeed(QStringLiteral("123456789"), &seed));
    EXPECT_EQ(seed, 123456789ULL);
    EXPECT_TRUE(GuiTraceSeed::parseFixedSeed(QStringLiteral("4294967295"), &seed));
    EXPECT_EQ(seed, 4294967295ULL);

    for (const char* invalid : {"", "-1", " 123", "123 ", "1.5",
                                "0x10", "4294967296", "18446744073709551615"}) {
        seed = 99;
        EXPECT_FALSE(GuiTraceSeed::parseFixedSeed(QString::fromLatin1(invalid), &seed))
            << invalid;
        EXPECT_EQ(seed, 99ULL);
    }
    EXPECT_FALSE(GuiTraceSeed::parseFixedSeed(QStringLiteral("1"), nullptr));
}

TEST(ScientificGuiSeed, FixedSelectionDoesNotChangeAutomaticMode)
{
    int automaticCalls = 0;
    auto automaticSeed = [&automaticCalls] {
        ++automaticCalls;
        return std::uint64_t{17};
    };
    const std::optional<std::uint64_t> automatic;
    EXPECT_EQ(GuiTraceSeed::resolve(automatic, automaticSeed), 17ULL);
    EXPECT_EQ(automaticCalls, 1);
    EXPECT_EQ(GuiTraceSeed::resolve(std::optional<std::uint64_t>{0}, automaticSeed), 0ULL);
    EXPECT_EQ(GuiTraceSeed::resolve(std::optional<std::uint64_t>{123456789ULL}, automaticSeed),
              123456789ULL);
    EXPECT_EQ(automaticCalls, 1);
}

TEST(ScientificGuiSeed, FixedSeedRepeatsThroughGuiStylePreparation)
{
    const std::uint64_t selectedSeed = GuiTraceSeed::resolve(
        std::optional<std::uint64_t>{987654321ULL},
        [] { return std::uint64_t{17}; });
    ScientificSignature first;
    ScientificSignature second;
    std::string error;
    constexpr ulong rays = 20001UL;
    ASSERT_TRUE(traceOnce(PreparationPath::GuiBorrowed, rays, false, &first, &error,
                          ScientificScene::CylinderVacuum, selectedSeed)) << error;
    ASSERT_TRUE(traceOnce(PreparationPath::GuiBorrowed, rays, false, &second, &error,
                          ScientificScene::CylinderVacuum, selectedSeed)) << error;
    expectValidTrace(first, rays);
    expectEqualScience(first, second);
}

#ifdef Q_OS_WIN
TEST(ScientificPluginPaths, WindowsBuildTreeIncludesMaterialPluginRoot)
{
    // Build-tree layout differs from installed layouts. The Windows exe is
    // under build/application/Release, but plugins are in build/plugins.
    const QDir pluginRoot(QString::fromUtf8(TONATIUHPP_SCIENTIFIC_PLUGIN_DIRECTORY));
    ASSERT_TRUE(pluginRoot.exists());

    const QString executableDir =
        pluginRoot.absoluteFilePath("../application/Release");
    const QString expected = QDir::cleanPath(pluginRoot.absolutePath());
    const QStringList searchPaths = TonatiuhCore::pluginSearchPaths(executableDir);

    bool found = false;
    for (const QString& directory : searchPaths) {
        if (QDir::cleanPath(QDir(directory).absolutePath()) == expected) {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found) << "Windows CMake application must find build/plugins: "
                       << expected.toStdString();
}
#endif

TEST(ScientificNativeTraceSignature, OrderIndependentAndSensitiveToCoordinates)
{
    QTemporaryDir temp;
    ASSERT_TRUE(temp.isValid());
    NativeTraceSignature::Inputs input;
    input.sceneFile = fixturePath();
    input.rays = 4;
    input.seed = kMasterSeed;
    input.sunGridWidth = 200;
    input.sunGridHeight = 200;
    input.apertureArea = 2.;
    input.irradiance = 1000.;
    input.powerPerRay = 500.;
    const RayTracerHit a{vec3d(0.25, 0.5, 0.125), nullptr, true};
    const RayTracerHit b{vec3d(0.75, 0.5, 0.125), nullptr, false};
    NativeTraceSignature first, reverse, changed;
    first.add(a);
    first.add(b);
    reverse.add(b);
    reverse.add(a);
    changed.add(a);
    changed.add(RayTracerHit{vec3d(0.751, 0.5, 0.125), nullptr, false});

    QString error;
    const QString p1 = temp.filePath("a.json");
    const QString p2 = temp.filePath("b.json");
    const QString p3 = temp.filePath("c.json");
    ASSERT_TRUE(first.write(p1, "native-gui", input, &error)) << error.toStdString();
    ASSERT_TRUE(reverse.write(p2, "headless-cli", input, &error)) << error.toStdString();
    ASSERT_TRUE(changed.write(p3, "headless-cli", input, &error)) << error.toStdString();

    auto read = [](const QString& path) {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly))
            return QJsonObject();
        return QJsonDocument::fromJson(file.readAll()).object();
    };
    const QJsonObject one = read(p1), two = read(p2), three = read(p3);
    ASSERT_FALSE(one.isEmpty());
    EXPECT_EQ(one.value("hit_sha256"), two.value("hit_sha256"));
    EXPECT_NE(one.value("hit_sha256"), three.value("hit_sha256"));
    EXPECT_EQ(one.value("scene_sha256"), two.value("scene_sha256"));
    EXPECT_EQ(one.value("hit_count").toString(), QStringLiteral("2"));
    NativeTraceSignature empty;
    EXPECT_FALSE(empty.write(temp.filePath("empty.json"), "headless-cli", input, &error));
    EXPECT_FALSE(NativeTraceSignature::validateRayCount(
        NativeTraceSignature::kMaxDiagnosticRays + 1, &error));
}

TEST(ScientificSimulationConfig, DefaultsAndValidation)
{
    SimulationConfig configuration;
    EXPECT_FALSE(HeadlessTracePreparationInput{}.updateTrackers);
    EXPECT_EQ(configuration.rays, 0UL);
    EXPECT_EQ(configuration.masterSeed, 0ULL);
    EXPECT_EQ(configuration.sunWidthDivisions, 200);
    EXPECT_EQ(configuration.sunHeightDivisions, 200);

    QString error;
    EXPECT_FALSE(configuration.validate(&error));
    EXPECT_EQ(error, QStringLiteral("Ray count must be greater than zero."));
    configuration.rays = 4096UL;
    configuration.sunWidthDivisions = 0;
    EXPECT_FALSE(configuration.validate(&error));
    EXPECT_EQ(error, QStringLiteral("Sun grid dimensions must be greater than zero."));
    configuration.sunWidthDivisions = 200;
    configuration.sunHeightDivisions = -1;
    EXPECT_FALSE(configuration.validate(&error));
    EXPECT_EQ(error, QStringLiteral("Sun grid dimensions must be greater than zero."));
    configuration.sunHeightDivisions = 200;
    EXPECT_TRUE(configuration.validate(&error));
    EXPECT_TRUE(error.isEmpty());
}

TEST(ScientificSimulationConfig, BothPreparationPathsRejectInvalidConfiguration)
{
    PreparedTraceContext context;
    GuiTracePreparationInput gui;
    HeadlessTracePreparationInput headless;
    QString error;
    EXPECT_FALSE(TracePreparation::prepareGuiTrace(gui, &context, &error));
    EXPECT_EQ(error, QStringLiteral("Ray count must be greater than zero."));
    EXPECT_FALSE(TracePreparation::prepareHeadlessTrace(headless, &context, &error));
    EXPECT_EQ(error, QStringLiteral("Ray count must be greater than zero."));
    gui.configuration.rays = 1UL;
    headless.configuration.rays = 1UL;
    gui.configuration.sunWidthDivisions = 0;
    headless.configuration.sunWidthDivisions = 0;
    EXPECT_FALSE(TracePreparation::prepareGuiTrace(gui, &context, &error));
    EXPECT_EQ(error, QStringLiteral("Sun grid dimensions must be greater than zero."));
    EXPECT_FALSE(TracePreparation::prepareHeadlessTrace(headless, &context, &error));
    EXPECT_EQ(error, QStringLiteral("Sun grid dimensions must be greater than zero."));
}

TEST(ScientificTracePreparation, VacuumExcludedAndExponentialRetained)
{
    LoadedScene loaded;
    QString error;
    ASSERT_TRUE(SceneLoader::readFile(fixturePath(), &loaded, &error))
        << error.toStdString();

    auto* vacuum = static_cast<AirTransmission*>(
        loaded.get()->getPart("world.air.transmission", false));
    ASSERT_NE(vacuum, nullptr);
    ASSERT_EQ(vacuum->getTypeId(), AirVacuum::getClassTypeId());
    EXPECT_EQ(TracePreparation::effectiveTracingAir(nullptr), nullptr);
    EXPECT_EQ(TracePreparation::effectiveTracingAir(vacuum), nullptr);

    // A non-vacuum atmosphere must retain its exact node and absorption.
    auto* atmosphere = new CountingExponentialAir;
    atmosphere->constant.setValue(0.35);
    ASSERT_TRUE(loaded.get()->setPart("world.air.transmission", atmosphere));
    auto* selected = static_cast<AirTransmission*>(
        loaded.get()->getPart("world.air.transmission", false));
    ASSERT_EQ(selected, atmosphere);
    EXPECT_EQ(TracePreparation::effectiveTracingAir(selected), selected);
    EXPECT_LT(selected->transmission(2.), 1.);
}

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);

    TonatiuhCore::initializeCoin();
    CorePluginRegistry plugins;
    // Use the known build-tree plugin root regardless of test executable
    // placement on Windows, macOS or Linux.
    plugins.loadScenePlugins({QString::fromUtf8(TONATIUHPP_SCIENTIFIC_PLUGIN_DIRECTORY)});
    TonatiuhCore::setProjectSearchPaths(fixturePath());
    return RUN_ALL_TESTS();
}
