#include "PluginManager.h"

#include <QDir>
#include <QPluginLoader>
#include <QStringList>

#include "application/core/TonatiuhCore.h"
#include "kernel/air/AirExponential.h"
#include "kernel/air/AirPolynomial.h"
#include "kernel/air/AirVacuum.h"
#include "kernel/component/ComponentFactory.h"
#include "kernel/material/MaterialAbsorber.h"
#include "kernel/material/MaterialFresnelUnpolarized.h"
#include "kernel/material/MaterialTransparent.h"
#include "kernel/material/MaterialVirtual.h"
#include "kernel/material/MaterialRough.h"
#include "kernel/photons/PhotonsAbstract.h"
#include "kernel/photons/PhotonsWidget.h"
#include "kernel/profiles/ProfileBox.h"
#include "kernel/profiles/ProfileCircular.h"
#include "kernel/profiles/ProfilePolygon.h"
#include "kernel/profiles/ProfileRectangular.h"
#include "kernel/profiles/ProfileRegular.h"
#include "kernel/profiles/ProfileTriangle.h"
#include "kernel/random/RandomSTL.h"
#include "kernel/scene/TSceneKit.h"
#include "kernel/scene/TSeparatorKit.h"
#include "kernel/scene/TShapeKit.h"
#include "kernel/shape/ShapeCone.h"
#include "kernel/shape/ShapeCube.h"
#include "kernel/shape/ShapeCylinder.h"
#include "kernel/shape/ShapeParabolic.h"
#include "kernel/shape/ShapePlanar.h"
#include "kernel/shape/ShapeSphere.h"
#include "kernel/sun/SunAperture.h"
#include "kernel/sun/SunKit.h"
#include "kernel/sun/SunShapePillbox.h"
#include "kernel/trackers/TrackerArmature.h"
#include "kernel/trackers/TrackerArmature1A.h"
#include "kernel/trackers/TrackerArmature2A.h"
#include "kernel/trackers/TrackerArmature2AwD.h"
#include "libraries/math/gcf.h"
#include "view/SkyNode3D.h"
#include "view/OverlayNode.h"
#include "view/SeparatorStyle.h"


PluginManager::PluginManager()
{
    TonatiuhCore::initializeCoreTypes();
    SkyNode3D::initClass();
    OverlayNode::initClass();
    SeparatorStyle::initClass();
}

/*!
 * Loads all the valid plugins from "plugins" subdirecotry of the directory in
 * which the running version of Tonatiuh is located.
 */
void PluginManager::load(QDir dir)
{
    loadPlugin(new SunFactoryT<SunShapePillbox>);

    loadPlugin(new AirFactoryT<AirVacuum>);
    loadPlugin(new AirFactoryT<AirExponential>);
    loadPlugin(new AirFactoryT<AirPolynomial>);

    loadPlugin(new ShapeFactoryT<ShapePlanar>);
    loadPlugin(new ShapeFactoryT<ShapeCube>);
    loadPlugin(new ShapeFactoryT<ShapeCone>);
    loadPlugin(new ShapeFactoryT<ShapeCylinder>);
    loadPlugin(new ShapeFactoryT<ShapeSphere>);
    loadPlugin(new ShapeFactoryT<ShapeParabolic>);

    loadPlugin(new ProfileFactoryT<ProfileBox>);
    loadPlugin(new ProfileFactoryT<ProfileRectangular>);
    loadPlugin(new ProfileFactoryT<ProfileCircular>);
    loadPlugin(new ProfileFactoryT<ProfileRegular>);
    loadPlugin(new ProfileFactoryT<ProfileTriangle>);
    loadPlugin(new ProfileFactoryT<ProfilePolygon>);

    loadPlugin(new MaterialFactoryT<MaterialAbsorber>);
    loadPlugin(new MaterialFactoryT<MaterialVirtual>);
    loadPlugin(new MaterialFactoryT<MaterialTransparent>);
    loadPlugin(new MaterialFactoryT<MaterialFresnelUnpolarized>);
    loadPlugin(new MaterialFactoryT<MaterialRough>);

    loadPlugin(new RandomFactoryT<RandomSTL>);

    loadPlugin(new TrackerFactoryT<TrackerArmature1A>);
    loadPlugin(new TrackerFactoryT<TrackerArmature2A>);
    loadPlugin(new TrackerFactoryT<TrackerArmature2AwD>);

    loadPlugin(new PhotonsFactoryT<PhotonsAbstract, PhotonsWidget>);

    QStringList files;
    findFiles(dir, files);

    for (QString file: files) {
        QPluginLoader loader(file);
        QObject* p = loader.instance();
        TFactory* f = dynamic_cast<TFactory*>(p);
        loadPlugin(f);
    }

    sort();
}


QVector<TFactory*> PluginManager::getFactories(SoNode* node)
{
    QVector<TFactory*> ans;
    if (dynamic_cast<ShapeRT*>(node)) {
        for (auto q : m_shapeFactories) ans << q;
    } else if (dynamic_cast<ProfileRT*>(node))
        for (auto q : m_profileFactories) ans << q;
    else if (dynamic_cast<MaterialRT*>(node))
        for (auto q : m_materialFactories) ans << q;
    else if (dynamic_cast<SunShape*>(node))
        for (auto q : m_sunFactories) ans << q;
    else if (dynamic_cast<AirTransmission*>(node))
        for (auto q : m_airFactories) ans << q;
    else if (dynamic_cast<TrackerArmature*>(node))
        for (auto q : m_trackerFactories) ans << q;
    return ans;
}

/*!
 * Creates a list with the files al files in the defined \a directory
 * and its subdirectories.
 */
void PluginManager::findFiles(QDir dir, QStringList& files)
{
    QString path = dir.absolutePath() + "/";

    for (QString f : dir.entryList(QDir::Files | QDir::NoSymLinks))
        files << path + f;

    for (QString d : dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot))
        findFiles(path + d, files);
}

/*!
 * Loads the plugin is defined in \a fileName, if it is a valid plugin.
 */
void PluginManager::loadPlugin(TFactory* p)
{
    if (auto f = dynamic_cast<AirFactory*>(p))
    {
        f->init();
        m_airFactories << f;
        m_airMap[f->name()] = f;
    }
    else if (auto f = dynamic_cast<ComponentFactory*>(p))
    {
        m_componentFactories << f;
    }
    else if (auto f = dynamic_cast<PhotonsFactory*>(p))
    {
        m_exportFactories << f;
        m_exportMap[f->name()] = f;
    }
    else if (auto f = dynamic_cast<MaterialFactory*>(p))
    {
        f->init();
        m_materialFactories << f;
        m_materialMap[f->name()] = f;
    }
    else if (auto f = dynamic_cast<ProfileFactory*>(p))
    {
        f->init();
        m_profileFactories << f;
        m_profileMap[f->name()] = f;
    }
    else if (auto f = dynamic_cast<RandomFactory*>(p))
    {
        m_randomFactories << f;
        m_randomMap[f->name()] = f;
    }
    else if (auto f = dynamic_cast<ShapeFactory*>(p))
    {
        f->init();
        m_shapeFactories << f;
        m_shapeMap[f->name()] = f;
    }
    else if (auto f = dynamic_cast<SunFactory*>(p))
    {
        f->init();
        m_sunFactories << f;
        m_sunMap[f->name()] = f;
    }
    else if (auto f = dynamic_cast<TrackerFactory*>(p))
    {
        f->init();
        m_trackerFactories << f;
        m_trackerMap[f->name()] = f;
    }
}

template <class T>
void sortFactories(const QStringList& sorting, QVector<T*>& factories) {
    int nA = 0;
    for (const QString& name : sorting) {
        if (name == "") {
            factories.append(nullptr);
            std::swap(factories[nA++], factories.last());
        } else {
            for (int n = 0; n < factories.size(); ++n) {
                if (factories[n] == nullptr) continue;
                if (factories[n]->name() == name) {
                    std::swap(factories[nA++], factories[n]);
                    break;
                }
            }
        }
    }
}

void PluginManager::sort()
{
    QStringList sunNames = {
        "Pillbox",
        "Buie"
    };
    sortFactories(sunNames, m_sunFactories);

    QStringList shapeNames = {
        "Planar",
        "Parabolic",
        "Hyperbolic",
        "Elliptic",
        "",
        "Cube",
        "Sphere",
        "Cylinder",
        "Cone",
        "",
        "Mesh",
        "MapN",
        "FunctionZ",
        "FunctionXYZ"
    };
    sortFactories(shapeNames, m_shapeFactories);

    QStringList materialNames = {
        "Transparent",
        "Virtual",
        "Absorber",
        "Specular",
        "Rough",
        "Fresnel (unpolarized)"
    };
    sortFactories(materialNames, m_materialFactories);

    QStringList trackerNames = {
        "one-axis",
        "two-axes"
    };
    sortFactories(trackerNames, m_trackerFactories);

    QStringList exportNames = {
        "No export",
        "File"
    };
    sortFactories(exportNames, m_exportFactories);

    QStringList randomNames = {
//        "Mersenne-Twister(STL)",
        "Mersenne-Twister"
    };
    sortFactories(randomNames, m_randomFactories);
}
