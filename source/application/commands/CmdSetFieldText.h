#pragma once

#include <QUndoCommand>

class SoNode;


class CmdSetFieldText: public QUndoCommand
{

public:
    CmdSetFieldText(SoNode* node, QString field, QString value, QUndoCommand* parent = 0);
    ~CmdSetFieldText();

    void undo();
    void redo();

private:
    SoNode* m_node;
    QString m_field;
    QString m_value;
    QString m_valueOld;
};
