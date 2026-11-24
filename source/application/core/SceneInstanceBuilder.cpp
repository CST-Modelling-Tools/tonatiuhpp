#include "SceneInstanceBuilder.h"

#include <Inventor/nodes/SoGroup.h>

#include "kernel/run/InstanceNode.h"
#include "kernel/scene/TSceneKit.h"
#include "kernel/scene/TSeparatorKit.h"

SceneInstanceTree SceneInstanceBuilder::build(TSceneKit* scene)
{
    SceneInstanceTree tree;
    if (!scene)
        return tree;

    tree.sceneRoot = std::make_unique<InstanceNode>(scene);
    TSeparatorKit* layout = scene->getLayout();
    if (layout)
        tree.layoutRoot = insertSubtree(tree.sceneRoot.get(), layout, 0);
    return tree;
}

InstanceNode* SceneInstanceBuilder::insertSubtree(InstanceNode* parent, SoNode* node, int row)
{
    if (!parent || !node)
        return nullptr;

    auto childOwner = std::make_unique<InstanceNode>(node);
    InstanceNode* instance = childOwner.get();
    parent->insertChild(row, instance);
    childOwner.release(); // The parent now owns this instance.

    // This is the sole recursive instance-construction path for both adapters.
    TSeparatorKit* kit = dynamic_cast<TSeparatorKit*>(node);
    SoGroup* group = kit ? static_cast<SoGroup*>(kit->getPart("group", false)) : nullptr;
    if (group) {
        for (int n = 0; n < group->getNumChildren(); ++n)
            insertSubtree(instance, group->getChild(n), n);
    }
    return instance;
}
