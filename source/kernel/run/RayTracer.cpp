#include <QPoint>
#include <QMutexLocker>
#include <type_traits>
#include <stdexcept>
#include "shape/DifferentialGeometry.h"
#include "random/Random.h"
#include "random/RussianRoulette.h"
#include "libraries/math/3D/Ray.h"
#include "RayTracer.h"
#include "kernel/run/InstanceNode.h"
#include "kernel/run/SceneBvh.h"
#include "kernel/photons/PhotonsBuffer.h"
#include "sun/SunAperture.h"
#include "sun/SunShape.h"
#include "air/AirTransmission.h"

namespace
{
// The no-history state is empty; only history-enabled tracing stores diagnostics.
struct NoRayHistoryState {};

struct CapturedRayHistoryState
{
    RayTracerHistory history;
    bool airAbsorbed = false;
    double airDistance = 0.;
};

void beginHistory(RayTracerHistory& history, std::uint64_t firstRayIndex,
                  ulong index, const Ray& ray)
{
    history.rayIndex = firstRayIndex + static_cast<std::uint64_t>(index);
    history.sunOrigin = ray.origin;
    history.sunDirection = ray.direction();
}

void reflectedHistory(RayTracerHistory& history, const Ray& ray,
                      const Ray& outgoing, InstanceNode* surface, bool front)
{
    history.steps.push_back(RayTracerHistoryStep{
        ray.point(ray.tMax), ray.origin, ray.direction(),
        outgoing.origin, outgoing.direction(), surface, front, true
    });
}

void endHistory(RayTracerHistory& history, const Ray& ray,
                InstanceNode* surface, bool front,
                bool airAbsorbed, double airDistance,
                const RayTracer::HistoryCallback& callback)
{
    const bool surfaceTerminated =
        !airAbsorbed && surface && ray.tMax != gcf::infinity;
    if (surfaceTerminated) {
        history.steps.push_back(RayTracerHistoryStep{
            ray.point(ray.tMax), ray.origin, ray.direction(),
            vec3d(), vec3d(), surface, front, false
        });
    }
    history.termination = airAbsorbed ? RayTracerHistoryTermination::Air
        : surfaceTerminated ? RayTracerHistoryTermination::Surface
        : RayTracerHistoryTermination::Escaped;
    history.terminalOrigin = ray.origin;
    history.terminalDirection = ray.direction();
    // In the air-loss branch tMax is reset to the infinity sentinel
    // after transmission. Preserve the actual sampled segment distance.
    history.terminalDistance = airAbsorbed ? airDistance : ray.tMax;
    callback(history);
}
} // namespace

RayTracer::RayTracer(const SceneBvh& sceneBvh,
    InstanceNode* instanceSun,
    SunAperture* sunAperture,
    SunShape* sunShape,
    AirTransmission* air,
    Random* rand,
    PhotonsBuffer* photonBuffer,
    QMutex* mutexPhotons,
    QVector<InstanceNode*> exportSuraceList,
    std::atomic_bool* exportFailed,
    HitCallback hitCallback,
    HistoryCallback historyCallback,
    std::uint64_t firstRayIndex
):
    m_sceneBvh(sceneBvh),
    m_instanceSun(instanceSun),
    m_sunAperture(sunAperture),
    m_sunShape(sunShape),
    m_sunTransform(instanceSun->getTransform()),
    m_air(air),
    m_rand(rand),
    m_photonBuffer(photonBuffer),
    m_mutexPhotonsBuffer(mutexPhotons),
    m_exportFailed(exportFailed),
    m_hitCallback(hitCallback),
    m_historyCallback(historyCallback),
    m_firstRayIndex(firstRayIndex),
    m_exportSurfaceList(exportSuraceList),
    m_sunCells(sunAperture->getCells())
{

}

// Select history policy once per tracing task, not once per ray.
// Both specializations share the single propagation implementation below.
void RayTracer::operator()(ulong nRays)
{
    m_energyBudget = {};
    if (m_historyCallback)
        traceRays<true>(nRays);
    else
        traceRays<false>(nRays);
}

template<bool CaptureHistory>
void RayTracer::traceRays(ulong nRays)
{
    if (m_sunCells.empty()) return;
    if (m_exportFailed && m_exportFailed->load())
        return;

    Random& rand = *m_rand;

    // The inactive specialization contains no history callback checks or
    // per-ray geometry-history state; both modes share physical propagation.
    using HistoryState = std::conditional_t<CaptureHistory,
        CapturedRayHistoryState, NoRayHistoryState>;

    // Both modes now use the same ray launch, intersection, air-transmission,
    // reflection and termination loop. Photon capture is observation only.
    const bool recordPhotons = m_photonBuffer && m_mutexPhotonsBuffer;
    const bool bExportAll = recordPhotons && m_exportSurfaceList.empty();
    const bool bExportLight = recordPhotons
        && (bExportAll || m_exportSurfaceList.contains(m_instanceSun));

    std::vector<Photon> photons;
    if (recordPhotons)
        photons.reserve(2*nRays);

    for (ulong n = 0; n < nRays; ++n) {
        if (m_exportFailed && m_exportFailed->load())
            return;

        Ray ray;
        NewPrimitiveRay(&ray, rand);
        ++m_energyBudget.launched;
        HistoryState historyState;
        if constexpr (CaptureHistory)
            beginHistory(historyState.history, m_firstRayIndex, n, ray);
        bool isFront = true;
        int rayLength = 0;
        InstanceNode* intersectedSurface = recordPhotons ? m_instanceSun : nullptr;

        if (bExportLight)
            photons.push_back(Photon(rayLength, ray.origin, m_instanceSun, isFront));

        bool isReflected = true;
        bool airAbsorbed = false;
        while (isReflected) {
            if (rayLength >= 1'000'000)
                throw std::runtime_error(
                    "Scientific integrity failure: ray exceeded one million interactions.");
            Ray rayReflected;
            isFront = false;
            intersectedSurface = nullptr;
            InstanceNode::Intersection hit;
            const bool hasHit = m_sceneBvh.findNearestHit(ray, hit);
            if (hasHit) {
                ray.tMax = hit.distance;
                isFront = hit.geometry.isFront;
                intersectedSurface = hit.instance;
            }

            // One validated atmospheric survival decision per eligible segment.
            // The direct solar leg is intentionally outside this contract.
            // Ground-level input DNI is assigned at the first optical surface;
            // do not apply atmosphere to the initial sun-to-surface leg.
            // Only finite post-first-hit paths between optical surfaces are
            // attenuated. A missed surface has no finite segment length: do
            // not evaluate a distance polynomial at infinity.
            if (m_air && rayLength > 0 && hasHit &&
                RussianRoulette::sample(rand, m_air->transmission(ray.tMax))
                    == RussianRoulette::Outcome::Absorbed) {
                airAbsorbed = true;
                if constexpr (CaptureHistory) {
                    historyState.airAbsorbed = true;
                    historyState.airDistance = ray.tMax;
                }
                // The recording path's terminal photon ID includes this air
                // interaction; the unrecorded path does not expose that ID.
                if (recordPhotons)
                    ++rayLength;
                intersectedSurface = nullptr;
                ray.tMax = gcf::infinity;
                break;
            }

            if (!hasHit)
                break;
            const MaterialInteraction event =
                InstanceNode::sampleMaterial(ray, hit, rand, rayReflected);
            isReflected = event != MaterialInteraction::Absorbed;
            if (!isReflected)
                break;

            if constexpr (CaptureHistory)
                reflectedHistory(historyState.history, ray, rayReflected, intersectedSurface, isFront);
            if (m_hitCallback && intersectedSurface)
                m_hitCallback(RayTracerHit{ray.point(ray.tMax), intersectedSurface, isFront});
            ++rayLength;
            if (recordPhotons && (bExportAll || m_exportSurfaceList.contains(intersectedSurface)))
                photons.push_back(Photon(rayLength, ray.point(ray.tMax), intersectedSurface, isFront, true));
            ray = rayReflected;
        }

        // Record one outcome for every physically completed propagation,
        // regardless of photon export and without extra random draws.
        m_energyBudget.record(airAbsorbed ? RayTerminalOutcome::Air
            : (intersectedSurface && ray.tMax != gcf::infinity)
                ? RayTerminalOutcome::SurfaceAbsorbed
                : RayTerminalOutcome::Escaped);

        // Preserve the M2.1-observed callback contract. The no-recording
        // terminal hit precedes history; recording emits history first, then
        // its legacy export-filtered terminal hit. Neither alters propagation.
        if (!recordPhotons) {
            if (m_hitCallback && intersectedSurface && ray.tMax != gcf::infinity)
                m_hitCallback(RayTracerHit{ray.point(ray.tMax), intersectedSurface, isFront});
            if constexpr (CaptureHistory)
                endHistory(historyState.history, ray, intersectedSurface, isFront,
                           historyState.airAbsorbed, historyState.airDistance, m_historyCallback);
            continue;
        }

        // Observe the physical terminal state before export-only tMax changes.
        if constexpr (CaptureHistory)
            endHistory(historyState.history, ray, intersectedSurface, isFront,
                       historyState.airAbsorbed, historyState.airDistance, m_historyCallback);

        // Retain the historical photon-selection and local path-ID policy.
        if (rayLength == 0 && ray.tMax == gcf::infinity)
            continue;
        if (!bExportAll && !m_exportSurfaceList.contains(intersectedSurface))
            continue;
        if (ray.tMax == gcf::infinity) {
            ray.tMax = 1.;
            isFront = false;
        }
        if (m_hitCallback && intersectedSurface)
            m_hitCallback(RayTracerHit{ray.point(ray.tMax), intersectedSurface, isFront});
        photons.push_back(Photon(++rayLength, ray.point(ray.tMax), intersectedSurface, isFront));
    }

    if (recordPhotons) {
        // Always unlock, even if a photon exporter throws.
        QMutexLocker lock(m_mutexPhotonsBuffer);
        const bool photonsSaved = m_photonBuffer->addPhotons(photons);
        if (!photonsSaved && m_exportFailed)
            m_exportFailed->store(true);
    }
}

bool RayTracer::NewPrimitiveRay(Ray* ray, Random& rand)
{
    int index = int(rand.RandomDouble()*m_sunCells.size());
    QPair<int, int> cell = m_sunCells[index];

    vec3d origin = m_sunAperture->Sample(rand.RandomDouble(), rand.RandomDouble(), cell.first, cell.second);
    vec3d direction = m_sunShape->generateRay(rand);
    *ray = m_sunTransform(Ray(origin, direction));
    return true;
}
