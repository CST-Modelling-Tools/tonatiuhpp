#pragma once

#include <QString>

class TSceneKit;

class LoadedScene
{
public:
    LoadedScene() = default;
    ~LoadedScene();

    LoadedScene(const LoadedScene&) = delete;
    LoadedScene& operator=(const LoadedScene&) = delete;

    TSceneKit* get() const { return m_scene; }
    TSceneKit* release();
    void reset(TSceneKit* scene = nullptr);

private:
    TSceneKit* m_scene = nullptr;
};

class SceneLoader
{
public:
    static bool readFile(const QString& fileName, LoadedScene* scene, QString* errorMessage = nullptr);
};
