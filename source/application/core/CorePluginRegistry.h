#pragma once

#include <QStringList>
#include <QVector>

class QPluginLoader;
class TFactory;

class CorePluginRegistry
{
public:
    CorePluginRegistry();
    ~CorePluginRegistry();

    void loadScenePlugins(const QStringList& directories);

private:
    void registerBuiltInSceneTypes();
    void findPluginFiles(const QString& directory, QStringList& files) const;
    bool loadPluginFile(const QString& fileName);
    bool registerSceneFactory(TFactory* factory, bool takeOwnership);

    QVector<TFactory*> m_ownedFactories;
    QVector<QPluginLoader*> m_pluginLoaders;
};
