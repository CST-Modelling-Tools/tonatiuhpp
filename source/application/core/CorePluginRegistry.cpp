#include "CorePluginRegistry.h"

#include <QLibrary>
#include <QPluginLoader>
#include <QDir>
#include <QFileInfo>
#include <QJsonObject>
#include <QObject>
#include <QSet>
#include <QtAlgorithms>

#include "core/TonatiuhCore.h"
#include "kernel/air/AirExponential.h"
#include "kernel/air/AirPolynomial.h"
#include "kernel/air/AirTransmission.h"
#include "kernel/air/AirVacuum.h"
#include "kernel/material/MaterialAbsorber.h"
#include "kernel/material/MaterialFresnelUnpolarized.h"
#include "kernel/material/MaterialRT.h"
#include "kernel/material/MaterialRough.h"
#include "kernel/material/MaterialTransparent.h"
#include "kernel/material/MaterialVirtual.h"
#include "kernel/node/TFactory.h"
#include "kernel/profiles/ProfileBox.h"
#include "kernel/profiles/ProfileCircular.h"
#include "kernel/profiles/ProfilePolygon.h"
#include "kernel/profiles/ProfileRectangular.h"
#include "kernel/profiles/ProfileRegular.h"
#include "kernel/profiles/ProfileTriangle.h"
#include "kernel/profiles/ProfileRT.h"
#include "kernel/scene/TShapeKit.h"
#include "kernel/shape/ShapeCone.h"
#include "kernel/shape/ShapeCube.h"
#include "kernel/shape/ShapeCylinder.h"
#include "kernel/shape/ShapeParabolic.h"
#include "kernel/shape/ShapePlanar.h"
#include "kernel/shape/ShapeRT.h"
#include "kernel/shape/ShapeSphere.h"
#include "kernel/sun/SunShape.h"
#include "kernel/sun/SunShapePillbox.h"
#include "kernel/trackers/TrackerArmature.h"
#include "kernel/trackers/TrackerArmature1A.h"
#include "kernel/trackers/TrackerArmature2A.h"
#include "kernel/trackers/TrackerArmature2AwD.h"

namespace
{
bool isSceneFactoryPlugin(const QString& fileName)
{
    QPluginLoader loader(fileName);
    const QString iid = loader.metaData().value("IID").toString();
    static const QSet<QString> sceneFactoryIids = {
        "tonatiuh.AirFactory",
        "tonatiuh.MaterialFactory",
        "tonatiuh.ProfileFactory",
        "tonatiuh.ShapeFactory",
        "tonatiuh.SunFactory",
        "tonatiuh.TrackerFactory",
    };

    return sceneFactoryIids.contains(iid);
}
}

CorePluginRegistry::CorePluginRegistry()
{
    TonatiuhCore::initializeCoreTypes();
    registerBuiltInSceneTypes();
}

CorePluginRegistry::~CorePluginRegistry()
{
    qDeleteAll(m_pluginLoaders);
    qDeleteAll(m_ownedFactories);
}

void CorePluginRegistry::loadScenePlugins(const QStringList& directories)
{
    QStringList files;
    for (const QString& directory : directories)
        findPluginFiles(directory, files);
    files.removeDuplicates();

    for (const QString& fileName : files)
        loadPluginFile(fileName);
}

void CorePluginRegistry::registerBuiltInSceneTypes()
{
    registerSceneFactory(new SunFactoryT<SunShapePillbox>, true);

    registerSceneFactory(new AirFactoryT<AirVacuum>, true);
    registerSceneFactory(new AirFactoryT<AirExponential>, true);
    registerSceneFactory(new AirFactoryT<AirPolynomial>, true);

    registerSceneFactory(new ShapeFactoryT<ShapePlanar>, true);
    registerSceneFactory(new ShapeFactoryT<ShapeCube>, true);
    registerSceneFactory(new ShapeFactoryT<ShapeCone>, true);
    registerSceneFactory(new ShapeFactoryT<ShapeCylinder>, true);
    registerSceneFactory(new ShapeFactoryT<ShapeSphere>, true);
    registerSceneFactory(new ShapeFactoryT<ShapeParabolic>, true);

    registerSceneFactory(new ProfileFactoryT<ProfileBox>, true);
    registerSceneFactory(new ProfileFactoryT<ProfileRectangular>, true);
    registerSceneFactory(new ProfileFactoryT<ProfileCircular>, true);
    registerSceneFactory(new ProfileFactoryT<ProfileRegular>, true);
    registerSceneFactory(new ProfileFactoryT<ProfileTriangle>, true);
    registerSceneFactory(new ProfileFactoryT<ProfilePolygon>, true);

    registerSceneFactory(new MaterialFactoryT<MaterialAbsorber>, true);
    registerSceneFactory(new MaterialFactoryT<MaterialVirtual>, true);
    registerSceneFactory(new MaterialFactoryT<MaterialTransparent>, true);
    registerSceneFactory(new MaterialFactoryT<MaterialFresnelUnpolarized>, true);
    registerSceneFactory(new MaterialFactoryT<MaterialRough>, true);

    registerSceneFactory(new TrackerFactoryT<TrackerArmature1A>, true);
    registerSceneFactory(new TrackerFactoryT<TrackerArmature2A>, true);
    registerSceneFactory(new TrackerFactoryT<TrackerArmature2AwD>, true);
}

void CorePluginRegistry::findPluginFiles(const QString& directory, QStringList& files) const
{
    QDir dir(directory);
    if (!dir.exists())
        return;

    const QFileInfoList entries = dir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QFileInfo& entry : entries) {
        if (entry.isDir()) {
            findPluginFiles(entry.absoluteFilePath(), files);
        } else if (QLibrary::isLibrary(entry.absoluteFilePath())) {
            files << entry.absoluteFilePath();
        }
    }
}

bool CorePluginRegistry::loadPluginFile(const QString& fileName)
{
    if (!isSceneFactoryPlugin(fileName))
        return false;

    QPluginLoader* loader = new QPluginLoader(fileName);
    QObject* plugin = loader->instance();
    if (!plugin) {
        delete loader;
        return false;
    }

    TFactory* factory = dynamic_cast<TFactory*>(plugin);
    if (!factory || !registerSceneFactory(factory, false)) {
        delete loader;
        return false;
    }

    m_pluginLoaders << loader;
    return true;
}

bool CorePluginRegistry::registerSceneFactory(TFactory* factory, bool takeOwnership)
{
    if (!factory)
        return false;

    bool registered = false;
    if (auto f = dynamic_cast<AirFactory*>(factory)) {
        f->init();
        registered = true;
    } else if (auto f = dynamic_cast<MaterialFactory*>(factory)) {
        f->init();
        registered = true;
    } else if (auto f = dynamic_cast<ProfileFactory*>(factory)) {
        f->init();
        registered = true;
    } else if (auto f = dynamic_cast<ShapeFactory*>(factory)) {
        f->init();
        registered = true;
    } else if (auto f = dynamic_cast<SunFactory*>(factory)) {
        f->init();
        registered = true;
    } else if (auto f = dynamic_cast<TrackerFactory*>(factory)) {
        f->init();
        registered = true;
    }

    if (registered && takeOwnership)
        m_ownedFactories << factory;
    else if (!registered && takeOwnership)
        delete factory;

    return registered;
}
