#pragma once

#include <QVector>
#include <QMap>
#include <QStringList>
#include <memory>
#include <vector>

class QPluginLoader;

class TFactory;
class SunFactory;
class AirFactory;
class ComponentFactory;
class TrackerFactory;
class ShapeFactory;
class ProfileFactory;
class MaterialFactory;
class PhotonsFactory;
class SoNode;

/*!
 * \class PluginManager
 * \brief PluginManager class manages plugin loading.
 * PluginManager is used to load plugins, manage the list of loaded plugins.
 */
class PluginManager
{

public:
    PluginManager();
    ~PluginManager();
    PluginManager(const PluginManager&) = delete;
    PluginManager& operator=(const PluginManager&) = delete;

    void load(const QStringList& pluginRoots);

    QVector<SunFactory*> getSunFactories() const {return m_sunFactories;}
    QVector<AirFactory*> getAirFactories() const {return m_airFactories;}
    QVector<ComponentFactory*> getComponentFactories() const {return m_componentFactories;}
    QVector<TrackerFactory*> getTrackerFactories() const {return m_trackerFactories;}
    QVector<ShapeFactory*> getShapeFactories() const {return m_shapeFactories;}
    QVector<ProfileFactory*> getProfileFactories() const {return m_profileFactories;}
    QVector<MaterialFactory*> getMaterialFactories() const {return m_materialFactories;}
    QVector<PhotonsFactory*> getExportFactories() const {return m_exportFactories;}

    const QMap<QString, SunFactory*>& getSunMap() const {return m_sunMap;}
    const QMap<QString, AirFactory*>& getAirMap() const {return m_airMap;}
    const QMap<QString, TrackerFactory*>& getTrackerMap() const {return m_trackerMap;}
    const QMap<QString, ShapeFactory*>& getShapeMap() const {return m_shapeMap;}
    const QMap<QString, ProfileFactory*>& getProfileMap() const {return m_profileMap;}
    const QMap<QString, MaterialFactory*>& getMaterialMap() const {return m_materialMap;}
    const QMap<QString, PhotonsFactory*>& getExportMap() const {return m_exportMap;}

    QVector<TFactory*> getFactories(SoNode* node);

private:
    // Built-ins belong to this manager; loaded plugin factories belong to
    // their retained QPluginLoader instances. Public maps borrow both.
    std::vector<std::unique_ptr<TFactory>> m_ownedFactories;
    std::vector<std::unique_ptr<QPluginLoader>> m_pluginLoaders;

    void loadPlugin(TFactory* p);
    void sort();

    QVector<SunFactory*> m_sunFactories;
    QVector<AirFactory*> m_airFactories;
    QVector<ComponentFactory*> m_componentFactories;
    QVector<TrackerFactory*> m_trackerFactories;
    QVector<ShapeFactory*> m_shapeFactories;
    QVector<ProfileFactory*> m_profileFactories;
    QVector<MaterialFactory*> m_materialFactories;
    QVector<PhotonsFactory*> m_exportFactories;

    QMap<QString, SunFactory*> m_sunMap;
    QMap<QString, AirFactory*> m_airMap;
    QMap<QString, TrackerFactory*> m_trackerMap;
    QMap<QString, ShapeFactory*> m_shapeMap;
    QMap<QString, ProfileFactory*> m_profileMap;
    QMap<QString, MaterialFactory*> m_materialMap;
    QMap<QString, PhotonsFactory*> m_exportMap;
};


