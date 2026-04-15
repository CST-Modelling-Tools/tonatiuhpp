#pragma once

#include "kernel/TonatiuhKernel.h"   // defines TONATIUH_KERNEL (and export macros)
#include "kernel/node/TNode.h"
#include "Photon.h"

#include <QStringList>
#include <vector>

struct PhotonsSettings;
class SceneTreeModel;   // <-- FIX: forward declaration
class PhotonsWidget;    // used later by PhotonsFactory
class PhotonsAbstract;  // used later by PhotonsFactory

class TONATIUH_KERNEL PhotonsAbstract
{
public:
    PhotonsAbstract();
    virtual ~PhotonsAbstract() {}

    virtual bool startExport() { return true; }
    virtual void savePhotons(const std::vector<Photon>& /*photons*/) {}
    virtual void setPhotonPower(double /*p*/) {}
    virtual void endExport() {}

    void setSceneModel(SceneTreeModel& sceneModel) { m_sceneModel = &sceneModel; }
    void setPhotonSettings(PhotonsSettings* ps);

    static QStringList getParameterNames() { return QStringList(); }
    virtual void setParameter(QString /*name*/, QString /*value*/) {}

    NAME_ICON_FUNCTIONS("No export", ":/photons/PhotonsDefault.png")

protected:
    SceneTreeModel* m_sceneModel = nullptr;   // <-- FIX: declared + default init

    bool m_saveAllPhotonsData = false;
    QStringList m_surfaces;

    bool m_saveCoordinates = false;
    bool m_saveCoordinatesGlobal = false;
    bool m_saveSurfaceID = false;
    bool m_saveSurfaceSide = false;
    bool m_savePhotonsID = false;
};

#include "kernel/node/TFactory.h"

class TONATIUH_KERNEL PhotonsFactory : public TFactory
{
public:
    virtual PhotonsAbstract* create(int) const = 0;
    virtual PhotonsWidget* createWidget() const { return nullptr; }
};

Q_DECLARE_INTERFACE(PhotonsFactory, "tonatiuh.PhotonsFactory")

template<class T, class W>
class PhotonsFactoryT : public PhotonsFactory
{
public:
    QString name() const { return T::getClassName(); }
    QIcon icon() const { return QIcon(T::getClassIcon()); }
    T* create(int) const { return new T; }
    W* createWidget() const { return new W; }
};
