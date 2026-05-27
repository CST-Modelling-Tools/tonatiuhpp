#pragma once

#include <QStringList>
#include <QTreeView>
#include <QVector>

#include "ParametersModel.h"

class SoNode;

class ParametersView: public QTreeView
{
    Q_OBJECT

public:
    ParametersView(QWidget* parent);

    ParametersModel* getModel();

    void setNode(SoNode* node);
    void reset();

private slots:
    void onPressed(const QModelIndex &index);

private:
    QVector<QStringList> expandedParameterPaths() const;
    void restoreExpandedParameterPaths(const QVector<QStringList>& paths);
    void expandDefaultParameterGroups();
    void expandParameterPath(const QStringList& path);
    void collectExpandedParameterPaths(const QModelIndex& parent, QStringList path, QVector<QStringList>* paths) const;
    QModelIndex findChildByName(const QModelIndex& parent, const QString& name) const;
    QString parameterName(const QModelIndex& index) const;
    void updateRootIndex();

    QVector<QStringList> m_expandedParameterPaths;
    bool m_hasExpansionPreference = false;
};
