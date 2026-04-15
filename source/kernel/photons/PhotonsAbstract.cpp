#include "PhotonsAbstract.h"

#include "PhotonsSettings.h"

PhotonsAbstract::PhotonsAbstract()
    : m_sceneModel(nullptr),
      m_saveAllPhotonsData(false),
      m_saveCoordinates(false),
      m_saveCoordinatesGlobal(true),
      m_saveSurfaceID(false),
      m_saveSurfaceSide(false),
      m_savePhotonsID(false)
{
}

void PhotonsAbstract::setPhotonSettings(PhotonsSettings* ps)
{
    if (!ps)
        return;

    m_surfaces = ps->surfaces;

    m_saveCoordinates       = ps->saveCoordinates;
    m_saveCoordinatesGlobal = ps->saveCoordinatesGlobal;
    m_saveSurfaceID         = ps->saveSurfaceID;
    m_saveSurfaceSide       = ps->saveSurfaceSide;
    m_savePhotonsID         = ps->savePhotonsID;

    for (auto it = ps->parameters.cbegin(); it != ps->parameters.cend(); ++it)
        setParameter(it.key(), it.value());
}
