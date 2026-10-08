#include <gtest/gtest.h>

#include <QCoreApplication>

#include <algorithm>
#include <array>
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
#include "kernel/air/AirTransmission.h"
#include "kernel/air/AirVacuum.h"
#include "kernel/photons/PhotonsBuffer.h"
#include "kernel/run/InstanceNode.h"
#include "kernel/run/RayTracer.h"
#include "kernel/scene/TSceneKit.h"
#include "kernel/sun/SunKit.h"

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

struct ScientificSignature
{
    RayTraceExecutorResult result;
    std::array<std::uint64_t, kHistogramSize> hitBins{};
    std::uint64_t hitCount = 0;
    std::uint64_t frontHitCount = 0;
    std::uint64_t invalidHitCount = 0;
    std::uint64_t recordedPhotonCount = 0;
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

// The GUI-style path intentionally models MainWindow::Run's scene-box sizing.
// It borrows a separate instance tree rather than constructing any GUI objects.
// This is NOT an end-to-end graphical UI test.
bool traceOnce(PreparationPath path, ulong rays, bool recordPhotons,
               ScientificSignature* signature, std::string* errorText)
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
    if (!SceneLoader::readFile(fixturePath(), &loaded, &error))
        return fail(error);

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
        sun->setBox(loaded.get()); // The UpdateLightSize() calculation in MainWindow.

        auto* air = static_cast<AirTransmission*>(
            loaded.get()->getPart("world.air.transmission", false));
        GuiTracePreparationInput input;
        input.scene = loaded.get();
        input.layoutRoot = borrowedTree.layoutRoot;
        input.sunInstance = &borrowedSun;
        input.masterSeed = kMasterSeed;
        input.photonBuffer = photonBuffer.get();
        input.tracingAir = air && air->getTypeId() != AirVacuum::getClassTypeId()
            ? air : nullptr;
        input.hitCallback = [&hits](const RayTracerHit& hit) { hits.add(hit); };
        input.rays = rays;
        input.sunWidthDivisions = kSunGridDivisions;
        input.sunHeightDivisions = kSunGridDivisions;
        if (!TracePreparation::prepareGuiTrace(input, &context, &error))
            return fail(error);
    } else {
        HeadlessTracePreparationInput input;
        input.scene = loaded.get();
        input.hitCallback = [&hits](const RayTracerHit& hit) { hits.add(hit); };
        input.rays = rays;
        input.seed = kMasterSeed;
        input.sunWidthDivisions = kSunGridDivisions;
        input.sunHeightDivisions = kSunGridDivisions;
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

// Opt in explicitly while GUI/headless sun-aperture equivalence is under review.
// A failure here is a diagnostic; do not change a scientific reference to hide it.
TEST(ScientificTraceDiagnostic, DISABLED_GuiStyleMatchesHeadless)
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

// Photon buffering exercises the other RayTracer propagation loop, without
// starting a file exporter. This is a diagnostic until those loops are unified.
TEST(ScientificTraceDiagnostic, DISABLED_PhotonRecordingPreservesScientificHits)
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

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);

    TonatiuhCore::initializeCoin();
    CorePluginRegistry plugins;
    TonatiuhCore::setProjectSearchPaths(fixturePath());
    return RUN_ALL_TESTS();
}
