#pragma once

#include <QStringList>
#include <QVector>

class QPluginLoader;
class SoNode;
class TFactory;

class CorePluginRegistry
{
public:
    CorePluginRegistry();
    ~CorePluginRegistry();

    void loadScenePlugins(const QStringList& directories);

    // Nonvisual factory creation, using the same names as the GUI factory maps.
    SoNode* createSceneNode(const QString& category, const QString& name) const;

private:
    void registerBuiltInSceneTypes();
    void findPluginFiles(const QString& directory, QStringList& files) const;
    bool loadPluginFile(const QString& fileName);
    bool registerSceneFactory(TFactory* factory, bool takeOwnership);

    QVector<TFactory*> m_ownedFactories;
    QVector<TFactory*> m_sceneFactories; // borrowed plugin entries or owned builtins
    QVector<QPluginLoader*> m_pluginLoaders;
};
