#pragma once

#include "kernel/photons/PhotonsBuffer.h"

// Nonvisual GUI trace-completion policy. The GUI owns progress, warnings and
// diagnostics; this helper owns the existing cumulative normalization and
// export acceptance decision after SimulationSession workers have joined.
struct GuiTraceCompletionResult
{
    ulong raysTracedTotal = 0;
    double photonPower = 0.;
    bool exportSucceeded = false;
    bool accepted = false;
};

namespace GuiTraceCompletion
{
inline GuiTraceCompletionResult finalize(PhotonsBuffer& buffer,
                                         bool traceCompleted,
                                         bool exportFailedDuringTrace,
                                         ulong previousRaysTraced,
                                         ulong completedRays,
                                         double apertureArea,
                                         double irradiance)
{
    GuiTraceCompletionResult result;
    result.raysTracedTotal = previousRaysTraced;
    if (traceCompleted)
        result.raysTracedTotal += completedRays;

    result.photonPower = result.raysTracedTotal > 0
        ? apertureArea * irradiance / result.raysTracedTotal : 0.;
    // Never normalize partial photons from an incomplete trace.
    result.exportSucceeded = buffer.endExport(traceCompleted ? result.photonPower : 0.);
    result.accepted = traceCompleted && !exportFailedDuringTrace && result.exportSucceeded;
    return result;
}
}
