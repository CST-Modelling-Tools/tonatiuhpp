#pragma once

#include <cstdint>

#include "core/TracePreparation.h"

// Pure, nonvisual adapter for GUI run inputs. All diagnostic callbacks stay
// in MainWindow and are installed on the returned object after construction.
namespace GuiTraceRunInputs
{
inline GuiTracePreparationInput make(TSceneKit* scene,
                                     InstanceNode* layoutRoot,
                                     InstanceNode* sunInstance,
                                     PhotonsBuffer* photonBuffer,
                                     const QVector<InstanceNode*>& exportSurfaces,
                                     AirTransmission* effectiveAir,
                                     std::uint64_t resolvedSeed,
                                     ulong rays,
                                     int gridWidth,
                                     int gridHeight)
{
    GuiTracePreparationInput input;
    input.scene = scene;
    input.layoutRoot = layoutRoot;
    input.sunInstance = sunInstance;
    input.photonBuffer = photonBuffer;
    input.exportSurfaceList = exportSurfaces;
    input.tracingAir = effectiveAir;
    input.configuration.masterSeed = resolvedSeed;
    input.configuration.rays = rays;
    input.configuration.sunWidthDivisions = gridWidth;
    input.configuration.sunHeightDivisions = gridHeight;
    return input;
}
}
