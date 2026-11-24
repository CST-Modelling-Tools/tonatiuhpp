#pragma once

#include <memory>

class InstanceNode;
class SoNode;
class TSceneKit;

struct SceneInstanceTree
{
    std::unique_ptr<InstanceNode> sceneRoot;
    InstanceNode* layoutRoot = nullptr;
};

class SceneInstanceBuilder
{
public:
    static SceneInstanceTree build(TSceneKit* scene);

    // Insert one complete occurrence (including nested groups) at the given
    // sibling row. The parent owns the new instance; each shared Coin node
    // receives its own InstanceNode occurrence and stable identity.
    // Returns nullptr when parent or node is null.
    static InstanceNode* insertSubtree(InstanceNode* parent, SoNode* node, int row);
};
