#include "CmdRename.h"

/*!
 * \class CmdChangeNodeName
 * \brief CmdChangeNodeName is the command that represents a node name change.
 *
 *CmdChangeNodeName represents a change in a node from the scene. The node can be any one.
 */

#include <QModelIndex>
#include <QString>

#include <Inventor/nodes/SoNode.h>

#include "libraries/math/gcf.h"
#include "kernel/run/InstanceNode.h"
#include "tree/SceneTreeModel.h"

CmdRename::CmdRename(
    const QModelIndex& index,
    QString name,
    SceneTreeModel* model,
    QUndoCommand* parent
):
    QUndoCommand(parent),
    m_name(name),
    m_model(model)
{
    if( !index.isValid() )
        gcf::SevereError("CmdChangeNodeName called with invalid ModelIndex.");

    InstanceNode* instance = m_model->getInstance(index);
    if (!instance)
        gcf::SevereError("CmdChangeNodeName called with invalid node." );

    m_node = instance->getNode();
    m_nameOld = m_node->getName().getString();
    // Undo history can outlive the corresponding GUI tree instance.
    m_node->ref();

    QString text = QString("node.setName(\"%1\");").arg(name);
    setText(text);
}

/*!
 * Changes the node name to the name that the node takes before the command is applied.
 */
CmdRename::~CmdRename()
{
    m_node->unref();
}

void CmdRename::undo()
{
    m_model->setNodeName(m_node, m_nameOld);
}

/*!
 * Changes the node name to the name defined in the command constructor.
 */
void CmdRename::redo()
{
    m_model->setNodeName(m_node, m_name);
}
