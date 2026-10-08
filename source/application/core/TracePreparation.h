#pragma once

#include <functional>
#include <cstdint>
#include <memory>

#include <QVector>
#include <QString>

#include "core/SceneInstanceBuilder.h"
#include "core/SimulationConfig.h"

class AirTransmission;
class InstanceNode;
class PhotonsBuffer;
class SunAperture;
class SunPosition;
class SunShape;
class TSceneKit;
struct RayTraceExecutorResult;
struct RayTracerHit;

using PreparedTraceHitCallback = std::function<void(const RayTracerHit&)>;
using TracePreparationProgress = std::function<void(const QString&)>;

struct GuiTracePreparationInput
{
    TSceneKit* scene = nullptr;
    InstanceNode* layoutRoot = nullptr;
    InstanceNode* sunInstance = nullptr;
    SimulationConfig configuration;
    PhotonsBuffer* photonBuffer = nullptr;
    QVector<InstanceNode*> exportSurfaceList;
    PreparedTraceHitCallback hitCallback;
    std::function<void()> synchronizeScene;
    AirTransmission* tracingAir = nullptr;
};

struct HeadlessTracePreparationInput
{
    TSceneKit* scene = nullptr;
    PreparedTraceHitCallback hitCallback;
    TracePreparationProgress progress;
    SimulationConfig configuration;
};

class PreparedTraceContext
{
public:
    PreparedTraceContext();
    ~PreparedTraceContext();
    PreparedTraceContext(const PreparedTraceContext&) = delete;
    PreparedTraceContext& operator=(const PreparedTraceContext&) = delete;
    PreparedTraceContext(PreparedTraceContext&&) noexcept;
    PreparedTraceContext& operator=(PreparedTraceContext&&) noexcept;

    ulong rays() const { return m_rays; }
    double sunApertureArea() const;
    double irradiance() const;
    double powerPerRay() const;
    PhotonsBuffer* photonBuffer() const { return m_photonBuffer; }

private:
    TSceneKit* m_scene = nullptr;
    SceneInstanceTree m_ownedInstanceTree;
    std::unique_ptr<InstanceNode> m_ownedSunInstance;

    InstanceNode* m_layoutRoot = nullptr;
    InstanceNode* m_sunInstance = nullptr;
    SunPosition* m_sunPosition = nullptr;
    SunAperture* m_sunAperture = nullptr;
    SunShape* m_sunShape = nullptr;
    AirTransmission* m_tracingAir = nullptr;
    std::uint64_t m_masterSeed = 0;
    PhotonsBuffer* m_photonBuffer = nullptr;
    QVector<InstanceNode*> m_exportSurfaceList;
    PreparedTraceHitCallback m_hitCallback;
    ulong m_rays = 0;

    friend class TracePreparation;
    friend class RayTraceExecutor;
};

class TracePreparation
{
public:
    static bool prepareGuiTrace(const GuiTracePreparationInput& input,
                                PreparedTraceContext* context,
                                QString* errorMessage = nullptr);
    static bool prepareHeadlessTrace(const HeadlessTracePreparationInput& input,
                                     PreparedTraceContext* context,
                                     QString* errorMessage = nullptr);
    static void initializeResult(const PreparedTraceContext& context, RayTraceExecutorResult* result);
    static bool finalizeResult(const PreparedTraceContext& context,
                               bool exportFailed,
                               double elapsedSeconds,
                               RayTraceExecutorResult* result,
                               QString* errorMessage = nullptr);
};
