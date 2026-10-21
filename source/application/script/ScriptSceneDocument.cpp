#include "ScriptSceneDocument.h"

#include <Inventor/nodes/SoGroup.h>

#include "kernel/scene/TSceneKit.h"
#include "core/TonatiuhCore.h"
#include "kernel/scene/TSeparatorKit.h"

ScriptSceneDocument::ScriptSceneDocument()
{
    TonatiuhCore::initializeCoin();
    TonatiuhCore::initializeCoreTypes();
    clear();
}

bool ScriptSceneDocument::clear(QString* errorMessage)
{
    if (errorMessage)
        errorMessage->clear();
    // LoadedScene::reset refs the new root and unreferences the old one.
    // Script NodeObject wrappers pin any previously exposed Coin nodes.
    m_scene.reset(new TSceneKit);
    return m_scene.get() != nullptr;
}

bool ScriptSceneDocument::insert(SoNode* node, QString* errorMessage)
{
    auto fail = [errorMessage](const QString& message) {
        if (errorMessage)
            *errorMessage = message;
        return false;
    };

    if (!node)
        return fail("Script document insertion requires a node.");
    TSceneKit* root = m_scene.get();
    TSeparatorKit* layout = root ? root->getLayout() : nullptr;
    if (!layout)
        return fail("Script document has no scene layout.");
    if (node == root || node == layout)
        return fail("Script document cannot insert its own scene root or layout.");
    auto* group = dynamic_cast<SoGroup*>(layout->getPart("group", true));
    if (!group)
        return fail("Script document layout has no child group.");

    // Match the GUI's root insertion when no scene-tree selection is active.
    // Coin's group takes a strong reference to the inserted node.
    group->addChild(node);
    root->updateParents(); // Establish tracker parent links, if present.
    if (errorMessage)
        errorMessage->clear();
    return true;
}
