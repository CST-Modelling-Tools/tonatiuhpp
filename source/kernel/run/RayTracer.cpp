#include <QPoint>
#include "shape/DifferentialGeometry.h"
#include "random/Random.h"
#include "libraries/math/3D/Ray.h"
#include "RayTracer.h"
#include "kernel/photons/PhotonsBuffer.h"
#include "sun/SunAperture.h"
#include "sun/SunShape.h"
#include "air/AirTransmission.h"

RayTracer::RayTracer(InstanceNode* instanceRoot,
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
    m_instanceLayout(instanceRoot),
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

void RayTracer::operator()(ulong nRays)
{
    if (m_sunCells.empty()) return;
    if (m_exportFailed && m_exportFailed->load())
        return;

    Random& rand = *m_rand;

    // The optional history hook observes complete per-ray propagation without
    // drawing random numbers, changing rays or depending on worker completion
    // order. It is never installed during normal tracing or frozen benchmarks.
    auto beginHistory = [this](RayTracerHistory& history, ulong index, const Ray& ray) {
        if (!m_historyCallback)
            return;
        history.rayIndex = m_firstRayIndex + static_cast<std::uint64_t>(index);
        history.sunOrigin = ray.origin;
        history.sunDirection = ray.direction();
    };
    auto reflectedHistory = [this](RayTracerHistory& history, const Ray& ray,
                                   const Ray& outgoing, InstanceNode* surface, bool front) {
        if (!m_historyCallback)
            return;
        history.steps.push_back(RayTracerHistoryStep{
            ray.point(ray.tMax), ray.origin, ray.direction(),
            outgoing.origin, outgoing.direction(), surface, front, true
        });
    };
    auto endHistory = [this](RayTracerHistory& history, const Ray& ray,
                             InstanceNode* surface, bool front,
                             bool airAbsorbed, double airDistance) {
        if (!m_historyCallback)
            return;
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
        m_historyCallback(history);
    };

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
        RayTracerHistory history;
        beginHistory(history, n, ray);
        bool isFront = true;
        int rayLength = 0;
        InstanceNode* intersectedSurface = recordPhotons ? m_instanceSun : nullptr;
        bool airAbsorbed = false;
        double airDistance = 0.;

        if (bExportLight)
            photons.push_back(Photon(rayLength, ray.origin, m_instanceSun, isFront));

        bool isReflected = true;
        while (isReflected) {
            Ray rayReflected;
            isFront = false;
            intersectedSurface = nullptr;
            isReflected = m_instanceLayout->intersect(ray, rand, isFront, intersectedSurface, rayReflected);

            // Air absorption is checked after the first reflection in both
            // modes, with exactly one RandomDouble draw per eligible segment.
            if (m_air && rayLength > 0 && m_air->transmission(ray.tMax) < rand.RandomDouble()) {
                if (m_historyCallback) {
                    airAbsorbed = true;
                    airDistance = ray.tMax;
                }
                // The recording path's terminal photon ID includes this air
                // interaction; the unrecorded path does not expose that ID.
                if (recordPhotons)
                    ++rayLength;
                intersectedSurface = nullptr;
                ray.tMax = gcf::infinity;
                break;
            }

            if (!isReflected)
                break;

            reflectedHistory(history, ray, rayReflected, intersectedSurface, isFront);
            if (m_hitCallback && intersectedSurface)
                m_hitCallback(RayTracerHit{ray.point(ray.tMax), intersectedSurface, isFront});
            ++rayLength;
            if (recordPhotons && (bExportAll || m_exportSurfaceList.contains(intersectedSurface)))
                photons.push_back(Photon(rayLength, ray.point(ray.tMax), intersectedSurface, isFront, true));
            ray = rayReflected;
        }

        // Preserve the M2.1-observed callback contract. The no-recording
        // terminal hit precedes history; recording emits history first, then
        // its legacy export-filtered terminal hit. Neither alters propagation.
        if (!recordPhotons) {
            if (m_hitCallback && intersectedSurface && ray.tMax != gcf::infinity)
                m_hitCallback(RayTracerHit{ray.point(ray.tMax), intersectedSurface, isFront});
            endHistory(history, ray, intersectedSurface, isFront, airAbsorbed, airDistance);
            continue;
        }

        // Observe the physical terminal state before export-only tMax changes.
        endHistory(history, ray, intersectedSurface, isFront, airAbsorbed, airDistance);

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
        m_mutexPhotonsBuffer->lock();
        const bool photonsSaved = m_photonBuffer->addPhotons(photons);
        m_mutexPhotonsBuffer->unlock();
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
