#pragma once

#include <mutex>
#include <vector>

#include <QString>

#include "core/NativeTraceSignature.h"
#include "kernel/run/RayTracer.h"

// Opt-in A0 diagnostic: collects complete ordered propagation histories for
// every launched ray. Stable ray indices remove worker-completion ordering.
class NativeRayHistorySignature final
{
public:
    static QString outputPathFromEnvironment();

    void add(const RayTracerHistory& history);
    bool write(const QString& fileName, const QString& mode,
               const NativeTraceSignature::Inputs& inputs,
               QString* errorMessage = nullptr) const;

private:
    mutable std::mutex m_mutex;
    std::vector<RayTracerHistory> m_histories;
};
