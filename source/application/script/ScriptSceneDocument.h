#pragma once

#include <QString>

#include "core/SceneLoader.h"

class SoNode;
class TSceneKit;

// A script-owned, GUI-independent document. LoadedScene pins the Coin root
// until it is replaced or this script host finishes. GUI scripts use their
// existing document/undo adapter instead of owning a second scene.
class ScriptSceneDocument final
{
public:
    ScriptSceneDocument();

    TSceneKit* scene() const { return m_scene.get(); }
    bool clear(QString* errorMessage = nullptr);
    bool insert(SoNode* node, QString* errorMessage = nullptr);

private:
    LoadedScene m_scene;
};
