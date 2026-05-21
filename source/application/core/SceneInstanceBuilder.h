#pragma once

#include <memory>

class InstanceNode;
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

private:
    static void generateInstanceTree(InstanceNode* instance);
};
