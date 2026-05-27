#include "ParametersView.h"

#include "ParametersDelegate.h"
#include <QApplication>
#include <QStandardItem>


ParametersView::ParametersView(QWidget* parent):
    QTreeView(parent)
{
    ParametersModel* model = new ParametersModel(this);
    setModel(model);

    setEditTriggers(QAbstractItemView::NoEditTriggers);

    setItemDelegate(new ParametersDelegate(this));

    connect(
        this, SIGNAL(pressed(QModelIndex)),
        this, SLOT(onPressed(QModelIndex))
    );

    setStyleSheet(R"(
QLineEdit {
    border: 1px solid #c8dbe5;
    selection-background-color: #c8dbe5;
    selection-color: black;
}

QComboBox QAbstractItemView::item {
  padding-bottom: 0.2ex;
}

    )");
// border: 2px solid darkgray;
// selection-background-color: lightgray;
//    QComboBox QAbstractItemView::item {
//      padding-bottom: 0.2ex;
//    }

//    QComboBox QAbstractItemView::separator {
//      padding-bottom: 0.2ex;
//    }

//    QApplication::setEffectEnabled(Qt::UI_AnimateCombo, false);
}

ParametersModel* ParametersView::getModel()
{
    return (ParametersModel*) model();
}

void ParametersView::setNode(SoNode* node)
{
    if (rootIndex().isValid()) {
        m_expandedParameterPaths = expandedParameterPaths();
        m_hasExpansionPreference = true;
    }

    getModel()->setNode(node);
    updateRootIndex();

    if (!rootIndex().isValid())
        return;

    if (m_hasExpansionPreference)
        restoreExpandedParameterPaths(m_expandedParameterPaths);
    else
        expandDefaultParameterGroups();
}

void ParametersView::reset()
{
    QTreeView::reset();
    double w = 2.5*fontMetrics().horizontalAdvance("Parameter");
    setColumnWidth(0, w);
    updateRootIndex();
}

QVector<QStringList> ParametersView::expandedParameterPaths() const
{
    QVector<QStringList> paths;
    collectExpandedParameterPaths(rootIndex(), QStringList(), &paths);
    return paths;
}

void ParametersView::restoreExpandedParameterPaths(const QVector<QStringList>& paths)
{
    for (const QStringList& path : paths)
        expandParameterPath(path);
}

void ParametersView::expandDefaultParameterGroups()
{
    expandParameterPath({"transform"});
    expandParameterPath({"translation"});
    expandParameterPath({"rotation"});
    expandParameterPath({"scale"});
    expandParameterPath({"scaleFactor"});
}

void ParametersView::expandParameterPath(const QStringList& path)
{
    QModelIndex parent = rootIndex();
    for (const QString& name : path) {
        const QModelIndex child = findChildByName(parent, name);
        if (!child.isValid())
            return;
        expand(child);
        parent = child;
    }
}

void ParametersView::collectExpandedParameterPaths(const QModelIndex& parent, QStringList path, QVector<QStringList>* paths) const
{
    if (!paths)
        return;
    const int rows = model() ? model()->rowCount(parent) : 0;
    for (int row = 0; row < rows; ++row) {
        const QModelIndex child = model()->index(row, 0, parent);
        if (!child.isValid() || !model()->hasChildren(child))
            continue;
        QStringList childPath = path;
        childPath << parameterName(child);
        if (!isExpanded(child))
            continue;
        paths->append(childPath);
        collectExpandedParameterPaths(child, childPath, paths);
    }
}

QModelIndex ParametersView::findChildByName(const QModelIndex& parent, const QString& name) const
{
    const int rows = model() ? model()->rowCount(parent) : 0;
    for (int row = 0; row < rows; ++row) {
        const QModelIndex child = model()->index(row, 0, parent);
        if (parameterName(child).compare(name, Qt::CaseInsensitive) == 0)
            return child;
    }
    return QModelIndex();
}

QString ParametersView::parameterName(const QModelIndex& index) const
{
    return model() ? model()->data(index, Qt::DisplayRole).toString() : QString();
}

void ParametersView::updateRootIndex()
{
    if (ParametersModel* m = getModel()) {
        QStandardItem* root = m->invisibleRootItem();
        root = root->child(0);
        setRootIndex(root ? root->index() : QModelIndex());
    }
}

void ParametersView::onPressed(const QModelIndex& index)
{
    if (model()->flags(index) & Qt::ItemIsEditable) {
//        setSelectionBehavior(QAbstractItemView::SelectItems);
//        selectionModel()->select(index, QItemSelectionModel::Clear);
        edit(index);
//        selectionModel()->select(index, QItemSelectionModel::Select);
//        setSelectionBehavior(QAbstractItemView::SelectRows);
    }
}

