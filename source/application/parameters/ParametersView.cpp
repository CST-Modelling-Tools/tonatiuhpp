#include "ParametersView.h"

#include "ParametersDelegate.h"
#include <QApplication>
#include <QHeaderView>
#include <QStandardItem>
#include <QTimer>

namespace
{
constexpr int kMinimumParameterColumnWidth = 180;
constexpr int kMaximumParameterColumnWidth = 320;
constexpr int kMinimumValueColumnWidth = 120;
const QString kPathSeparator = QStringLiteral("/");
}


ParametersView::ParametersView(QWidget* parent):
    QTreeView(parent)
{
    ParametersModel* model = new ParametersModel(this);
    setModel(model);
    header()->setStretchLastSection(true);
    header()->setSectionResizeMode(0, QHeaderView::Interactive);
    header()->setSectionResizeMode(1, QHeaderView::Stretch);

    setEditTriggers(QAbstractItemView::NoEditTriggers);

    setItemDelegate(new ParametersDelegate(this));

    connect(
        this, SIGNAL(pressed(QModelIndex)),
        this, SLOT(onPressed(QModelIndex))
    );
    connect(this, &QTreeView::expanded, this, &ParametersView::onExpanded);
    connect(this, &QTreeView::collapsed, this, &ParametersView::onCollapsed);

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
    const int generation = ++m_restoreGeneration;
    m_ignoreExpansionSignals = true;
    getModel()->setNode(node);
    updateRootIndex();

    if (!rootIndex().isValid()) {
        m_ignoreExpansionSignals = false;
        return;
    }

    const QPersistentModelIndex newRoot(rootIndex());
    QTimer::singleShot(0, this, [this, newRoot, generation]() {
        restoreExpansionAndColumnWidth(newRoot, generation);
    });
}

void ParametersView::reset()
{
    QTreeView::reset();
    updateRootIndex();
}

void ParametersView::restoreExpansionAndColumnWidth(QPersistentModelIndex rootIndex, int generation)
{
    if (generation != m_restoreGeneration)
        return;

    m_ignoreExpansionSignals = false;
    if (!rootIndex.isValid() || rootIndex != this->rootIndex())
        return;

    const int restoredCount = restoreStoredExpansion();
    if (restoredCount == 0)
        expandDefaultParameterGroups();

    applyColumnWidth();
}

int ParametersView::restoreStoredExpansion()
{
    int restoredCount = 0;
    for (const QString& key : m_expandedPathKeys) {
        const QStringList path = pathFromKey(key);
        if (hasCollapsedPreference(path))
            continue;
        const QModelIndex before = findChildByName(rootIndex(), path.value(0));
        expandParameterPath(path);
        if (before.isValid())
            ++restoredCount;
    }
    return restoredCount;
}

void ParametersView::applyColumnWidth()
{
    resizeColumnToContents(0);
    int width = columnWidth(0);
    width = qMax(width, kMinimumParameterColumnWidth);
    width = qMin(width, qMax(kMinimumParameterColumnWidth, viewport()->width() - kMinimumValueColumnWidth));
    width = qMin(width, kMaximumParameterColumnWidth);
    setColumnWidth(0, width);
}

void ParametersView::expandDefaultParameterGroups()
{
    expandParameterPath({"transform"});
    expandParameterPath({"transform", "translation"});
    expandParameterPath({"transform", "rotation"});
    expandParameterPath({"transform", "scale"});
    expandParameterPath({"transform", "scaleFactor"});
    expandParameterPath({"translation"});
    expandParameterPath({"rotation"});
    expandParameterPath({"scale"});
    expandParameterPath({"scaleFactor"});
}

void ParametersView::expandParameterPath(const QStringList& path)
{
    if (path.isEmpty() || hasCollapsedPreference(path))
        return;

    QModelIndex parent = rootIndex();
    for (const QString& name : path) {
        const QModelIndex child = findChildByName(parent, name);
        if (!child.isValid())
            return;
        expand(child);
        parent = child;
    }
}

bool ParametersView::hasCollapsedPreference(const QStringList& path) const
{
    for (int length = 1; length <= path.size(); ++length) {
        if (m_collapsedPathKeys.contains(pathKey(path.mid(0, length))))
            return true;
    }
    return false;
}

QStringList ParametersView::parameterPath(const QModelIndex& index) const
{
    QStringList path;
    QModelIndex current = index;
    while (current.isValid() && current != rootIndex()) {
        path.prepend(parameterName(current));
        current = current.parent();
    }
    return path;
}

QString ParametersView::pathKey(const QStringList& path) const
{
    return path.join(kPathSeparator);
}

QStringList ParametersView::pathFromKey(const QString& key) const
{
    return key.split(kPathSeparator, Qt::SkipEmptyParts);
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

void ParametersView::onExpanded(const QModelIndex& index)
{
    if (m_ignoreExpansionSignals)
        return;
    const QStringList path = parameterPath(index);
    if (path.isEmpty())
        return;
    const QString key = pathKey(path);
    m_collapsedPathKeys.remove(key);
    m_expandedPathKeys.insert(key);
}

void ParametersView::onCollapsed(const QModelIndex& index)
{
    if (m_ignoreExpansionSignals)
        return;
    const QStringList path = parameterPath(index);
    if (path.isEmpty())
        return;
    const QString key = pathKey(path);
    m_expandedPathKeys.remove(key);
    m_collapsedPathKeys.insert(key);
}

