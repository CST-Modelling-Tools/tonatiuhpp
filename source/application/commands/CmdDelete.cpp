#include "CmdDelete.h"

#include <Inventor/nodekits/SoBaseKit.h>

#include "tree/SceneTreeModel.h"
#include "kernel/run/InstanceNode.h"


CmdDelete::CmdDelete(const QModelIndex& index, SceneTreeModel* model, QUndoCommand* parent):
    QUndoCommand("Delete", parent),
    m_model(model)
{
    InstanceNode* instance = m_model->getInstance(index);
    m_node = instance->getNode();
    m_node->ref();

    InstanceNode* instanceP = instance->getParent();
    m_parent = static_cast<SoBaseKit*>(instanceP->getNode());
    m_row = instanceP->indexOfChild(instance);
}

CmdDelete::~CmdDelete()
{
    m_node->unref();
}

void CmdDelete::undo()
{
    m_model->Paste(m_parent, m_node, m_row, true);
}

void CmdDelete::redo()
{
    m_model->Cut(m_parent, m_row);
}
