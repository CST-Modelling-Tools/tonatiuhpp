#include <Inventor/nodekits/SoBaseKit.h>

#include "libraries/math/gcf.h"

#include "CmdPaste.h"
#include "tree/SceneTreeModel.h"
#include "kernel/run/InstanceNode.h"

/**
 * Creates a new paste command that represents a \a type paste of the \a coinClipboard node as a node located with \a parentModelIndex child in the \a model.
 *
 * If \a parent is not null, this command is appended to parent's child list and then owns this command.
 */
CmdPaste::CmdPaste(bool isShared,
                   const QModelIndex& index,
                   SoNode*& node,
                   SceneTreeModel* model,
                   QUndoCommand* parent):
    QUndoCommand("Paste", parent),
    m_isShared(isShared),
    m_row(-1),
    m_node(node),
    m_model(model)
{
    m_name = node->getName().getString();

    if (!index.isValid())
        gcf::SevereError("CmdPaste called with invalid ModelIndex.");

    InstanceNode* parentInstance = m_model->getInstance(index);
    if (!parentInstance || !parentInstance->getNode())
        gcf::SevereError("CmdPaste NULL m_coinParent.");

    m_parentNode = dynamic_cast<SoBaseKit*>(parentInstance->getNode());
    if (!m_parentNode)
        gcf::SevereError("CmdPaste requires a Coin node-kit parent.");
    m_row = parentInstance->childCount();
    // Undo of an ancestor insert can replace its InstanceNode wrapper.
    // Pin the Coin parent and source rather than that transient wrapper.
    m_parentNode->ref();
    m_node->ref();
}

CmdPaste::~CmdPaste()
{
    if (m_pastedNode)
        m_pastedNode->unref();
    m_node->unref();
    m_parentNode->unref();
}

void CmdPaste::undo()
{
    if (!m_pastedNode || !m_parentNode)
        return;
    // The model validates the exact Coin child before removing a row.
    // Keep the copy alive: later undo commands may still refer to its fields.
    if (!m_model->Cut(m_parentNode, m_row, m_pastedNode))
        return;
    m_model->setNodeName(m_node, m_name);
}

void CmdPaste::redo()
{
    if (!m_parentNode || !m_node)
        return;

    // First execution copies (or links) the clipboard node. Subsequent redos
    // reuse that SAME Coin node, preserving field pointers in later commands.
    SoNode* source = m_pastedNode ? m_pastedNode : m_node;
    const bool share = m_pastedNode || m_isShared;
    SoNode* insertedNode = nullptr;
    if (!m_model->Paste(m_parentNode, source, m_row, share, &insertedNode)
        || !insertedNode)
        return;
    if (!m_pastedNode) {
        m_pastedNode = insertedNode;
        m_pastedNode->ref();
    } else if (m_pastedNode != insertedNode) {
        // Never retarget command history to a different node identity.
        return;
    }

    QString name = m_name;
    int count = 0;
    while (!m_model->setNodeName(m_pastedNode, name))
        name = m_name + QString("_%1").arg(++count);
}
