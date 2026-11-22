#pragma once

#include <QUndoCommand>

class QString;
class SoNode;
class SceneTreeModel;


class CmdRename: public QUndoCommand
{
public:
    CmdRename(const QModelIndex& index, QString name, SceneTreeModel* model, QUndoCommand* parent = 0);
    ~CmdRename();

    void undo();
    void redo();

private:
    QString m_name;
    QString m_nameOld;
    SoNode* m_node;
    SceneTreeModel* m_model;
};
