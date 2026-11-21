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
#include "core/ScientificFactoryCatalog.h"
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
    for (const QString& fileName : TonatiuhCore::pluginLibraryFiles(directories))
        loadPluginFile(fileName);
}

void CorePluginRegistry::registerBuiltInSceneTypes()
{
    // The catalog transfers ownership of these factories to this registry.
    std::vector<std::unique_ptr<TFactory>> factories =
        ScientificFactoryCatalog::createBuiltInFactories();
    for (std::unique_ptr<TFactory>& factory : factories) {
        if (registerSceneFactory(factory.get(), false)) {
            m_ownedFactories.append(factory.get());
            factory.release();
        }
    }
}

bool CorePluginRegistry::loadPluginFile(const QString& fileName)
{
    if (!isSceneFactoryPlugin(fileName))
        return false;

    QPluginLoader* loader = new QPluginLoader(fileName);
    if (!TonatiuhCore::allowsPluginMetadata(loader->metaData())) {
        delete loader;
        return false;
    }
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

    if (registered)
        m_sceneFactories << factory;
    if (registered && takeOwnership)
        m_ownedFactories << factory;
    else if (!registered && takeOwnership)
        delete factory;

    return registered;
}

SoNode* CorePluginRegistry::createSceneNode(const QString& category, const QString& name) const
{
    // Later registered plugins override same-name builtins like GUI maps do.
    for (auto it = m_sceneFactories.crbegin(); it != m_sceneFactories.crend(); ++it) {
        TFactory* factory = *it;
        if (!factory || factory->name() != name)
            continue;
        if (category == "shape") {
            if (auto* f = dynamic_cast<ShapeFactory*>(factory)) return f->create();
        } else if (category == "profile") {
            if (auto* f = dynamic_cast<ProfileFactory*>(factory)) return f->create();
        } else if (category == "material") {
            if (auto* f = dynamic_cast<MaterialFactory*>(factory)) return f->create();
        } else if (category == "tracker") {
            if (auto* f = dynamic_cast<TrackerFactory*>(factory)) return f->create();
        } else if (category == "sun") {
            if (auto* f = dynamic_cast<SunFactory*>(factory)) return f->create();
        } else if (category == "air") {
            if (auto* f = dynamic_cast<AirFactory*>(factory)) return f->create();
        }
    }
    return nullptr;
}
