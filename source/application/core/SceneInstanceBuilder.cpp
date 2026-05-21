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
    if (!layout)
        return tree;

    tree.layoutRoot = new InstanceNode(layout);
    tree.sceneRoot->addChild(tree.layoutRoot);
    generateInstanceTree(tree.layoutRoot);
    return tree;
}

void SceneInstanceBuilder::generateInstanceTree(InstanceNode* instance)
{
    if (!instance || !instance->getNode())
        return;

    TSeparatorKit* kit = dynamic_cast<TSeparatorKit*>(instance->getNode());
    if (!kit)
        return;

    SoGroup* group = static_cast<SoGroup*>(kit->getPart("group", false));
    if (!group)
        return;

    for (int n = 0; n < group->getNumChildren(); ++n) {
        SoNode* childNode = group->getChild(n);
        InstanceNode* childInstance = new InstanceNode(childNode);
        instance->addChild(childInstance);
        generateInstanceTree(childInstance);
    }
}
