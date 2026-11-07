#pragma once

#include <optional>

#include <QMap>
#include <QString>
#include <QStringList>

// Phase 3B2A: portable description of script-requested photon-export settings.
// This holds only explicit changes; it does NOT instantiate export plugins,
// start a simulation, normalize power, or replace a PhotonsBuffer. Those
// operations remain guarded in the historical GUI path until Phase 3B2B.
struct ScriptPhotonExportChange
{
    enum class Kind {
        AllSurfaces, Coordinates, SurfaceID, SurfaceSide, MapType,
        PhotonsID, Parameter, AddSurface, BufferSize, BufferAppend
    };
    Kind kind;
    bool flag = false;
    bool aux = false;
    uint count = 0;
    QString name;
    QString value;
};

struct ScriptPhotonExportState
{
    bool settingsPresent = false;
    std::optional<QString> exporterName;
    std::optional<bool> saveCoordinates;
    std::optional<bool> saveCoordinatesGlobal;
    std::optional<bool> saveSurfaceID;
    std::optional<bool> saveSurfaceSide;
    std::optional<bool> savePhotonsID;
    QStringList surfaces;
    QMap<QString, QString> parameters;
    std::optional<uint> bufferSize;
    std::optional<bool> bufferAppend;

    void apply(const ScriptPhotonExportChange& change)
    {
        using Kind = ScriptPhotonExportChange::Kind;
        switch (change.kind) {
        case Kind::AllSurfaces: surfaces.clear(); break;
        case Kind::Coordinates:
            saveCoordinates = change.flag;
            saveCoordinatesGlobal = change.aux;
            break;
        case Kind::SurfaceID: saveSurfaceID = change.flag; break;
        case Kind::SurfaceSide: saveSurfaceSide = change.flag; break;
        case Kind::MapType:
            exporterName = change.name;
            settingsPresent = true;
            break;
        case Kind::PhotonsID: savePhotonsID = change.flag; break;
        case Kind::Parameter: parameters.insert(change.name, change.value); break;
        case Kind::AddSurface: surfaces.append(change.name); break;
        case Kind::BufferSize: bufferSize = change.count; break;
        case Kind::BufferAppend: bufferAppend = change.flag; break;
        }
    }
};
