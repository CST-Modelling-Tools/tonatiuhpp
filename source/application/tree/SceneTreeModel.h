#pragma once

#include <QAbstractItemModel>
#include <QSet>
#include <Inventor/SoType.h>

class InstanceNode;
class QModelIndex;
class SoBaseKit;
class SoNode;
class SoNodeKitPath;
class SoSeparator;
class SunKit;
class TSeparatorKit;
class TSceneKit;
class TShapeKit;
class Document;
class AirKit;


class SceneTreeModel: public QAbstractItemModel
{
    Q_OBJECT

public:
    SceneTreeModel(QObject* parent = 0);
    ~SceneTreeModel();

    void clear();

    void setDocument(Document* document);
    TSceneKit* getSceneKit() const {return m_nodeScene;}

    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const;
    int rowCount(const QModelIndex& index) const;
    int columnCount(const QModelIndex& index) const;
    QModelIndex parent(const QModelIndex& index) const;
    QVariant data(const QModelIndex& modelIndex, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    Qt::ItemFlags flags(const QModelIndex& index) const;
    Qt::DropActions supportedDropActions() const;
    Qt::DropActions supportedDragActions() const;

    InstanceNode* getInstance(const QModelIndex& index) const;
    bool setNodeName(SoNode* node, QString name);
    bool setNodeNameUnique(SoNode* node, QString name);
    QModelIndex indexFromUrl(QString url) const;
    QModelIndex indexFromPath(const SoNodeKitPath& path) const;
    SoNodeKitPath* pathFromIndex(const QModelIndex& index) const;

    bool hasChild(SoType type, SoBaseKit* parent);
    int insertCoinNode(SoNode* coinChild, SoBaseKit* parent);
    void removeCoinNode(int row, SoBaseKit* parent);

    bool Cut(SoBaseKit* parent, int row);
    bool Paste(SoBaseKit* parent, SoNode* node, int row, bool isShared = false);

    void UpdateSceneModel();

private:
    void clearInstanceTree();
    InstanceNode* addInstanceNode(InstanceNode* parent, SoNode* node);
    void generateInstanceTree(InstanceNode* instance);
    void deleteInstanceTree(InstanceNode* instance);
    bool containsInstance(InstanceNode* instance) const;

private:
    SoSeparator* m_nodeRoot;
    TSceneKit* m_nodeScene; // child of root
    InstanceNode* m_instanceScene;
    InstanceNode* m_instanceLayout; // child of scene

    // the same SoNode can be attached to multiple parents
    // InstanceNode have a single parent
    std::map< SoNode*, QList<InstanceNode*> > m_mapCoinQt;
    QSet<InstanceNode*> m_instances;
};
