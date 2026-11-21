#include "CmdCut.h"

#include <Inventor/nodekits/SoBaseKit.h>

#include "tree/SceneTreeModel.h"
#include "kernel/run/InstanceNode.h"


CmdCut::CmdCut(const QModelIndex& index, SoNode*& clipboard, SceneTreeModel* model, QUndoCommand* parent):
    QUndoCommand("Cut", parent),
    m_clipboard(clipboard),
    m_model(model)
{
    m_nodeOld = clipboard;

    InstanceNode* instance = m_model->getInstance(index);
    m_node = instance->getNode();
    m_node->ref();

    InstanceNode* instanceP = instance->getParent();
    m_parent = static_cast<SoBaseKit*>(instanceP->getNode());
    m_row = instanceP->indexOfChild(instance);
}

CmdCut::~CmdCut()
{
    m_node->unref();
}

void CmdCut::undo()
{
    m_clipboard = m_nodeOld;
    m_model->Paste(m_parent, m_node, m_row, true);
}

void CmdCut::redo()
{
    m_clipboard = m_node;
    m_model->Cut(m_parent, m_row);
}
