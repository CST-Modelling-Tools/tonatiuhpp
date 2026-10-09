#include <gtest/gtest.h>

#include <QCoreApplication>

#include <Inventor/actions/SoGetBoundingBoxAction.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>

#include "core/CorePluginRegistry.h"
#include "core/RayTraceExecutor.h"
#include "core/SceneInstanceBuilder.h"
#include "core/SceneLoader.h"
#include "core/TonatiuhCore.h"
#include "core/TracePreparation.h"
#include "run/GuiTraceSeed.h"
#include "kernel/air/AirExponential.h"
#include "kernel/air/AirTransmission.h"
#include "kernel/air/AirVacuum.h"
#include "kernel/photons/PhotonsBuffer.h"
#include "kernel/run/InstanceNode.h"
#include "kernel/run/RayTracer.h"
#include "kernel/scene/TSceneKit.h"
#include "kernel/scene/TSeparatorKit.h"
#include "kernel/scene/TShapeKit.h"
#include "kernel/sun/SunAperture.h"
#include "kernel/sun/SunKit.h"
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

enum class ScientificScene
{
    CylinderVacuum,
    FresnelTwoSurfaceVacuum,
    FresnelTwoSurfaceExponentialAir
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

struct ScientificSignature
{
    RayTraceExecutorResult result;
    std::array<std::uint64_t, kHistogramSize> hitBins{};
    std::uint64_t hitCount = 0;
    std::uint64_t frontHitCount = 0;
    std::uint64_t invalidHitCount = 0;
    std::uint64_t recordedPhotonCount = 0;
    std::uint64_t airTransmissionCalls = 0;
    std::uint64_t shapeInstanceCount = 0;
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
    }

    void copyTo(ScientificSignature* destination) const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        destination->hitCount = m_signature.hitCount;
        destination->frontHitCount = m_signature.frontHitCount;
        destination->invalidHitCount = m_signature.invalidHitCount;
        destination->hitBins = m_signature.hitBins;
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
    return scene == ScientificScene::CylinderVacuum
        ? fixturePath()
        : QString::fromUtf8(TONATIUHPP_FRESNEL_SCENE_FILE);
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
bool traceOnce(PreparationPath path, ulong rays, bool recordPhotons,
               ScientificSignature* signature, std::string* errorText,
               ScientificScene scene = ScientificScene::CylinderVacuum,
               std::uint64_t masterSeed = kMasterSeed)
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

    LoadedScene loaded;
    QString error;
    if (!SceneLoader::readFile(fixturePath(scene), &loaded, &error))
        return fail(error);

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
        input.tracingAir = air && air->getTypeId() != AirVacuum::getClassTypeId()
            ? air : nullptr;
        input.hitCallback = [&hits](const RayTracerHit& hit) { hits.add(hit); };
        input.configuration.rays = rays;
        input.configuration.sunWidthDivisions = kSunGridDivisions;
        input.configuration.sunHeightDivisions = kSunGridDivisions;
        if (!TracePreparation::prepareGuiTrace(input, &context, &error))
            return fail(error);
    } else {
        HeadlessTracePreparationInput input;
        input.scene = loaded.get();
        input.hitCallback = [&hits](const RayTracerHit& hit) { hits.add(hit); };
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
    if (photonBuffer)
        signature->recordedPhotonCount = photonBuffer->getPhotons().size();
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

} // namespace

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

TEST(ScientificSimulationConfig, DefaultsAndValidation)
{
    SimulationConfig configuration;
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

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);

    TonatiuhCore::initializeCoin();
    CorePluginRegistry plugins;
    TonatiuhCore::setProjectSearchPaths(fixturePath());
    return RUN_ALL_TESTS();
}
