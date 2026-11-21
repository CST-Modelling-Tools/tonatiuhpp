#include "CmdSetFieldText.h"

#include <Inventor/nodes/SoNode.h>
#include <Inventor/fields/SoField.h>
#include <Inventor/SbString.h>

#include "libraries/math/gcf.h"


CmdSetFieldText::CmdSetFieldText(SoNode* node, QString field, QString value, QUndoCommand* parent):
    QUndoCommand(parent),
    m_node(node),
    m_field(field),
    m_value(value)
{
    if (!m_node)
        gcf::SevereError("CmdParameterModified called with invalid node.");

    SoField* f = m_node->getField(m_field.toLatin1().data());
    if (!f)
        gcf::SevereError("CmdParameterModified called with invalid parameter.");

    SbString valueOld;
    f->get(valueOld);
    m_valueOld = valueOld.getString();
    // A later Undo may detach the owning scene kit while this command
    // remains in QUndoStack; keep the field-owner node alive.
    m_node->ref();

//    QString name = node->getTypeId().getName().getString();
//    QString text = QString("%1.%2 = %3").arg(name, m_field, m_value);
    QString text = QString("node.setParameter(\"%1\", \"%2\");").arg(m_field, m_value);
    setText(text);
}

CmdSetFieldText::~CmdSetFieldText()
{
    m_node->unref();
}

void CmdSetFieldText::undo()
{
    SoField* f = m_node->getField(m_field.toLatin1().data());
    f->set(m_valueOld.toLatin1().data());
}

void CmdSetFieldText::redo()
{
    SoField* f = m_node->getField(m_field.toLatin1().data());
    f->set(m_value.toLatin1().data());
}
