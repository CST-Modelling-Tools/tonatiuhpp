#pragma once

#include <array>
#include <cstdint>
#include <mutex>
#include <vector>

#include <QString>
#include <qglobal.h>

struct RayTracerHit;

// Opt-in A0 diagnostic only. Never attached to production ray tracing unless
// TONATIUHPP_A0_TRACE_SIGNATURE_FILE is set. Comparisons are within-platform.
class NativeTraceSignature final
{
public:
    static constexpr ulong kMaxDiagnosticRays = 200'000UL;

    struct Inputs
    {
        QString sceneFile;
        ulong rays = 0;
        std::uint64_t seed = 0;
        int sunGridWidth = 0;
        int sunGridHeight = 0;
        double apertureArea = 0.;
        double irradiance = 0.;
        double powerPerRay = 0.;
    };

    static QString outputPathFromEnvironment();
    static bool validateRayCount(ulong rays, QString* errorMessage);

    void add(const RayTracerHit& hit);
    bool write(const QString& fileName, const QString& mode,
               const Inputs& inputs, QString* errorMessage = nullptr) const;

private:
    struct Event
    {
        QString surfaceUrl;
        bool isFront = false;
        std::array<std::uint64_t, 3> positionBits{};

        bool operator<(const Event& other) const;
    };

    mutable std::mutex m_mutex;
    std::vector<Event> m_events;
    std::uint64_t m_nonfiniteCount = 0;
};
