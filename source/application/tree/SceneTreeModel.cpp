#include "SceneTreeModel.h"

#include <QIcon>
#include <QMessageBox>

#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/actions/SoSearchAction.h>
#include <Inventor/nodekits/SoBaseKit.h>
#include <Inventor/nodekits/SoNodeKitListPart.h>
#include <Inventor/nodekits/SoSceneKit.h>
#include <Inventor/nodes/SoSelection.h>
#include <Inventor/nodes/SoMaterial.h>

#include "application/view/GraphicRoot.h"
#include "kernel/profiles/ProfileRT.h"
#include "kernel/material/MaterialRT.h"
#include "kernel/run/InstanceNode.h"
#include "kernel/scene/TSceneKit.h"
#include "kernel/trackers/TrackerKit.h"
#include "kernel/scene/TSeparatorKit.h"
#include "kernel/scene/TShapeKit.h"
#include "kernel/scene/TArrayKit.h"
#include "kernel/scene/WorldKit.h"
#include "kernel/scene/LocationNode.h"
#include "kernel/scene/TerrainKit.h"
#include "kernel/shape/ShapeRT.h"
#include "kernel/air/AirKit.h"
#include "kernel/sun/SunKit.h"
#include "kernel/sun/SunKit.h"
#include "kernel/trackers/TrackerKit.h"
#include "libraries/math/gcf.h"
#include "tree/SoPathVariant.h"
#include "main/Document.h"


SceneTreeModel::SceneTreeModel(QObject* parent):
    QAbstractItemModel(parent),
    m_nodeRoot(0),
    m_nodeScene(0),
    m_instanceScene(0),
    m_instanceLayout(0)
{

}

SceneTreeModel::~SceneTreeModel()
{
    clearInstanceTree();
}

/*!
 * Clears the model scene
 *
 * Clears nodes from the model.
 */
void SceneTreeModel::clear()
{
    if (!m_instanceScene && m_mapCoinQt.empty() && m_instances.isEmpty()) return;

    beginResetModel();
    clearInstanceTree();
    endResetModel();
}

void SceneTreeModel::clearInstanceTree()
{
    deleteInstanceTree(m_instanceScene);

    delete m_instanceScene;
    m_instanceScene = 0;
    m_instanceLayout = 0;
    m_nodeRoot = 0;
    m_nodeScene = 0;
    m_mapCoinQt.clear();
    m_instances.clear();
}

/*!
 * Sets the model scene to the given \a coinScene.
 *
 * Creates nodes for the model to the scene subnodes.
 */
void SceneTreeModel::setDocument(Document* document)
{
    beginResetModel();

    clearInstanceTree();
    if (!document || !document->getSceneKit())
    {
        endResetModel();
        return;
    }

    m_nodeRoot = document->getSceneKit()->m_graphicRoot->getRoot();
    m_nodeScene = document->getSceneKit();

    m_instanceScene = new InstanceNode(m_nodeScene);
    m_mapCoinQt[m_nodeScene].append(m_instanceScene);
    m_instances.insert(m_instanceScene);

//    SoNode* worldKit = m_nodeScene->getPart("world", true);
//    InstanceNode* instanceW = addInstanceNode(m_instanceScene, worldKit);

//    SoNode* location = m_nodeScene->getPart("world.location", true);
//    addInstanceNode(instanceW, location);

//    SoNode* sunKitW = m_nodeScene->getPart("world.sun", true);
//    addInstanceNode(instanceW, sunKitW);

//    SoNode* airKit = m_nodeScene->getPart("world.air", true);
//    addInstanceNode(instanceW, airKit);

//    SoNode* terrainKit = m_nodeScene->getPart("world.terrain", true);
//    addInstanceNode(instanceW, terrainKit);

    TSeparatorKit* nodeLayout = m_nodeScene->getLayout();
    if (nodeLayout)
    {
        m_instanceLayout = addInstanceNode(m_instanceScene, nodeLayout);
        generateInstanceTree(m_instanceLayout);
    }

    endResetModel();
}

InstanceNode* SceneTreeModel::addInstanceNode(InstanceNode* parent, SoNode* node)
{
    if (!parent || !node) return 0;

    InstanceNode* instance = new InstanceNode(node);
    parent->addChild(instance);
    m_mapCoinQt[node].append(instance);
    m_instances.insert(instance);
    return instance;
}

void SceneTreeModel::generateInstanceTree(InstanceNode* instance)
{
    if (!instance) return;

    SoNode* node = instance->getNode();
    if (!node) return;

    if (TSeparatorKit* kit = dynamic_cast<TSeparatorKit*>(node))
    {
        SoGroup* group = (SoGroup*) kit->getPart("group", false);
        if (!group) return;
        for (int n = 0; n < group->getNumChildren(); ++n)
        {
            SoNode* childNode = (SoNode*) group->getChild(n);
            InstanceNode* childInstance = addInstanceNode(instance, childNode);
            generateInstanceTree(childInstance);
        }
    }
}

void SceneTreeModel::deleteInstanceTree(InstanceNode* instance)
{
    if (!instance) return;

    while (instance->children.count() > 0)
    {
        InstanceNode* child = instance->children[instance->children.count() - 1];
        instance->children.remove(instance->children.count() - 1);
        if (!child) continue;

        child->setParent(0);
        deleteInstanceTree(child);
        delete child;
    }

//    for (InstanceNode* child : instance.children)
//    {
//        DeleteInstanceTree(*child);
//        delete child;
//    }

    std::map<SoNode*, QList<InstanceNode*> >::iterator it = m_mapCoinQt.find(instance->getNode());
    if (it != m_mapCoinQt.end())
    {
        QList<InstanceNode*>& instanceList = it->second;
        int q = instanceList.indexOf(instance);
        if (q >= 0) instanceList.removeAt(q);
        if (instanceList.isEmpty()) m_mapCoinQt.erase(it);
    }

    InstanceNode* instanceParent = instance->getParent();
    if (instanceParent)
    {
        int row = instanceParent->children.indexOf(instance);
        if (row >= 0) instanceParent->children.remove(row);
        instance->setParent(0);
    }

    m_instances.remove(instance);
}

QModelIndex SceneTreeModel::index(int row, int column, const QModelIndex& parent) const
{
    if (column != 0 || row < 0) return QModelIndex();

    InstanceNode* instance = getInstance(parent);
    if (!instance) return QModelIndex();
    if (row >= instance->children.count()) return QModelIndex();

    InstanceNode* child = instance->children[row];
    if (!containsInstance(child)) return QModelIndex();
    if (child->getParent() != instance) return QModelIndex();

    return createIndex(row, column, instance->children[row]);
}

int SceneTreeModel::rowCount(const QModelIndex& index) const
{
    if (index.isValid() && index.column() != 0) return 0;

    InstanceNode* instance = getInstance(index);
    return instance ? instance->children.count() : 0;
}

int SceneTreeModel::columnCount(const QModelIndex& /*index*/) const
{
    return 1;
}

QModelIndex SceneTreeModel::parent(const QModelIndex& index) const
{
    if (!index.isValid() || index.model() != this || index.column() != 0) return QModelIndex();

    InstanceNode* instance = getInstance(index);
    if (!instance) return QModelIndex();

    InstanceNode* instanceParent = instance->getParent();
    if (!instanceParent) return QModelIndex();
    if (!containsInstance(instanceParent)) return QModelIndex();
    if (!instanceParent->children.contains(instance)) return QModelIndex();

    InstanceNode* instanceGrandparent = instanceParent->getParent();
    if (!instanceGrandparent) return QModelIndex();
    if (!containsInstance(instanceGrandparent)) return QModelIndex();

    int row = instanceGrandparent->children.indexOf(instanceParent);
    if (row < 0) return QModelIndex();

    return createIndex(row, 0, instanceParent);
}

QVariant SceneTreeModel::data(const QModelIndex& index, int role) const
{
    if (role != Qt::DisplayRole && role != Qt::UserRole && role != Qt::DecorationRole) return QVariant();
    if (!index.isValid() || index.model() != this || index.column() != 0) return QVariant();

    InstanceNode* instance = getInstance(index);
    if (!instance) return QVariant();

    SoNode* node = instance->getNode();
    if (!node) return QVariant();

    if (role == Qt::DisplayRole)
    {
        QString name;
        if (node->getName() == SbName())
            name = node->getTypeId().getName().getString();
        else
            name = node->getName().getString();

        SoSearchAction action;
        action.setNode(node);
        action.setInterest(SoSearchAction::ALL);
        if (m_nodeScene) action.apply(m_nodeScene);

        int count = action.getPaths().getLength();
//        count = node->getRefCount();
        if (count > 1)
            name = QString("%1 [%2]").arg(name).arg(count);

        return name;
    }
    if (role == Qt::UserRole)
    {
        SoNodeKitPath* path = pathFromIndex(index);
        SoPathVariant pathWrapper(path);
        return QVariant::fromValue(pathWrapper);
    }
    if (role == Qt::DecorationRole)
    {
        SoType type = node->getTypeId();
        if (type.isDerivedFrom(TSeparatorKit::getClassTypeId()))
        {
            return QIcon(":/images/scene/nodeGroup.png");
        }
        else if (type.isDerivedFrom(TShapeKit::getClassTypeId()))
        {
            return QIcon(":/images/scene/nodeShape.png");
        }
        else if (type.isDerivedFrom(TrackerKit::getClassTypeId()))
        {
            return QIcon(":/images/scene/nodeTracker.png");
        }
        else if (type.isDerivedFrom(TArrayKit::getClassTypeId()))
        {
            return QIcon(":/images/scene/nodeArray.png");
        }

        //        else if (type.isDerivedFrom(ShapeRT::getClassTypeId()))
//        {
//            ShapeRT* shape = static_cast<ShapeRT*>(node);
//            return QIcon(shape->getTypeIcon());
//        }
    }

    return QVariant();
}

QVariant SceneTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal) {
        if (role == Qt::DisplayRole) {
            if (section == 0) return "Node";
        }
    }
    return QVariant();
}

Qt::ItemFlags SceneTreeModel::flags(const QModelIndex& index) const
{
    Qt::ItemFlags defaultFlags = QAbstractItemModel::flags(index);
    if (!index.isValid()) return defaultFlags;
    if (index.column() > 0) return defaultFlags;
    return Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | Qt::ItemIsEditable | defaultFlags;
}

Qt::DropActions SceneTreeModel::supportedDropActions() const
{
    return Qt::CopyAction | Qt::MoveAction;
}

Qt::DropActions SceneTreeModel::supportedDragActions() const
{
    return Qt::CopyAction | Qt::MoveAction;
}

InstanceNode* SceneTreeModel::getInstance(const QModelIndex& index) const
{
    if (!index.isValid())
        return containsInstance(m_instanceScene) ? m_instanceScene : 0;
    if (index.model() != this) return 0;

    InstanceNode* instance = (InstanceNode*) index.internalPointer();
    return containsInstance(instance) ? instance : 0;
}

bool SceneTreeModel::containsInstance(InstanceNode* instance) const
{
    return instance && m_instances.contains(instance);
}

bool SceneTreeModel::setNodeName(SoNode* node, QString name)
{
    SbName sbname(name.toStdString().c_str());

    for (InstanceNode* instance : m_mapCoinQt[node])
    {
        for (InstanceNode* sibling : instance->getParent()->children)
        {
            if (sibling == instance) continue;
            if (sibling->getNode()->getName() == sbname)
                return false;
        }
    }

    node->setName(sbname);
    emit layoutChanged();
    return true;
}

bool SceneTreeModel::setNodeNameUnique(SoNode* node, QString name)
{
    QString nameU = name;
    int count = 0;
    while (!setNodeName(node, nameU))
        nameU = name + "_" + QString::number(++count);
    return true;
}

/**
 * Returns the index of item with the \a nodeUrl.
 *
 * If \a nodeUrl is not a valid node url, the function returns root node index.
 *
 *
 * \sa IndexFromNodeUrl, NodeFromIndex, PathFromIndex.
**/
QModelIndex SceneTreeModel::indexFromUrl(QString url) const
{
    QStringList path = url.split("/", Qt::SkipEmptyParts);

    if (path.size() == 0) return QModelIndex();

    QString nodeName = path.last();
    path.removeLast();
    QString parentURL = QString("//") + path.join("/");
    QModelIndex parentIndex = indexFromUrl(parentURL);
    InstanceNode* instanceParent = getInstance(parentIndex);
    if (!instanceParent) return QModelIndex();

    int row = 0;
    for (InstanceNode* child : instanceParent->children)
    {
        if (containsInstance(child) && child->getNode()
            && child->getNode()->getName() == nodeName.toStdString().c_str())
        {
            return index(row, 0, parentIndex);
        }
        row++;
    }

    return QModelIndex();
}

/**
 * Returns the index of item with the \a coinNodePath.
 *
 * If \a coinNodePath is not a valid node path, the function returns root node index.
 *
 *
 * \sa IndexFromNodeUrl, NodeFromIndex, PathFromIndex.
**/
#include <QDebug>
QModelIndex SceneTreeModel::indexFromPath(const SoNodeKitPath& path) const
{
    if (!m_instanceScene) return QModelIndex();

//    qDebug() << "asd";
//    for (int q = 0; q < path.getLength(); q++)
//    {
//        qDebug() << path.getNodeFromTail(q)->getName().getString() << "/";
//    }
//    qDebug() <<"fin";

    SoBaseKit* coinNode = static_cast<SoBaseKit*>(path.getTail());
    if (!coinNode) gcf::SevereError("IndexFromPath Null coinNode.");

    if (path.getLength() <= 1) return QModelIndex();
    int temp = 1;
    SoBaseKit* coinParent = static_cast<SoBaseKit*>(path.getNodeFromTail(temp));
    if (!coinParent) return QModelIndex();
//        gcf::SevereError("IndexFromPath Null coinParent.");

    if (coinParent->getTypeId().isDerivedFrom(SoSceneKit::getClassTypeId()))
    {
        int row = 0;
        for (InstanceNode* child : m_instanceScene->children)
        {
            if (containsInstance(child) && child->getNode() == coinNode)
                return index(row, 0);
            row++;
        }
        return QModelIndex();
    }


    while (coinParent->getTypeId().isDerivedFrom(SoShapeKit::getClassTypeId()))
    {
        coinNode = coinParent;
        coinParent = (SoBaseKit*) path.getNodeFromTail(++temp);
        if (!coinParent) return QModelIndex();
    }
    if (coinParent->getTypeId().isDerivedFrom(TShapeKit::getClassTypeId())
        || coinParent->getTypeId().isDerivedFrom(TrackerKit::getClassTypeId()))
    {
        coinNode = coinParent;
        coinParent = (SoBaseKit*) path.getNodeFromTail(++temp);
        if (!coinParent) return QModelIndex();
        if (coinParent->getTypeId().isDerivedFrom(TSeparatorKit::getClassTypeId()))
        {
            coinNode = coinParent;
            coinParent = (SoBaseKit*) path.getNodeFromTail(++temp);
            if (!coinParent) return QModelIndex();
        }
    }

    SoGroup* coinPartList = static_cast<SoGroup*>(coinParent->getPart("group", true));
    if (!coinPartList)
        return QModelIndex(); // todo
//        gcf::SevereError("IndexFromPath Null coinPartList.");
    int row = coinPartList->findChild(coinNode);

    SoNodeKitPath* pathParent = static_cast<SoNodeKitPath*>(path.copy());
    pathParent->ref();
    pathParent->truncate(pathParent->getLength() - temp);
    QModelIndex indexParent = indexFromPath(*pathParent);
    pathParent->unref();
    return index(row, 0, indexParent);
}

SoNodeKitPath* SceneTreeModel::pathFromIndex(const QModelIndex& index) const
{
    InstanceNode* instance = getInstance(index);
    if (!instance || !m_nodeRoot) return 0;

    // search for kits only
    SoNode* node = instance->getNode();
    if (!node) return 0;

    if (!node->getTypeId().isDerivedFrom(SoBaseKit::getClassTypeId()))
    {
        QModelIndex parentIndex = parent(index);
        if (!parentIndex.isValid()) return 0;
        return pathFromIndex(parentIndex);
    }

    // find first
    SoSearchAction action;
    action.setNode(node);
    action.setInterest(SoSearchAction::FIRST);
    action.apply(m_nodeRoot);

    SoNodeKitPath* path = (SoNodeKitPath*) action.getPath();
    if (!path) return 0;
    path->ref(); // otherwise destroyed with action

    //
//    SoNodeList nodeList;
//    InstanceNode* instance = getInstance(index);
//    while (instance->getParent())
//    {
//        nodeList.append(instance->getNode());
//        instance = instance->getParent();
//    }

//    for (int i = nodeList.getLength() - 1; i >= 0; --i)
//    {
//        SoBaseKit* coinNode = static_cast<SoBaseKit*>(nodeList[i]);
//        if (coinNode) path->append(coinNode);
//    }

    return path; // make unref later
}

bool SceneTreeModel::hasChild(SoType type, SoBaseKit* parent)
{
    TSeparatorKit* kit = dynamic_cast<TSeparatorKit*>(parent);
    if (!kit) return false;

    SoGroup* group = (SoGroup*) parent->getPart("group", true);
    for (int n = 0; n < group->getNumChildren(); ++n)
        if (group->getChild(n)->getTypeId() == type)  return true;
    return false;
}

int SceneTreeModel::insertCoinNode(SoNode* node, SoBaseKit* parent)
{
    int row = -1;
    if (dynamic_cast<TSeparatorKit*>(parent))
    {
            SoGroup* group = (SoGroup*) parent->getPart("group", true);
            row = group->getNumChildren();

//            if (SoBaseKit* kit = dynamic_cast<SoBaseKit*>(node))
//            {
//                kit->setSearchingChildren(true);
                group->addChild(node);
//            }
    }

    for (InstanceNode* instanceParent : m_mapCoinQt[parent])
    {
        InstanceNode* instance = new InstanceNode(node);
        instanceParent->insertChild(row, instance);
        m_mapCoinQt[node].append(instance);
        m_instances.insert(instance);
        generateInstanceTree(instance);
    }

    emit layoutChanged();
    return row;
}

void SceneTreeModel::removeCoinNode(int row, SoBaseKit* parent)
{
    if (parent->getTypeId().isDerivedFrom(TSeparatorKit::getClassTypeId()))
    {
         if (SoGroup* parts = (SoGroup*) parent->getPart("group", false))
            parts->removeChild(row);
    }

    for (InstanceNode* instanceParent : m_mapCoinQt[parent])
    {
        InstanceNode* instance = instanceParent->children[row];
        deleteInstanceTree(instance);
        delete instance;
    }

    emit layoutChanged();
}

/**
 * Cuts the \a row child from \a coinParent node.
 *
 * Returns whether the cut is successfully done.
**/
bool SceneTreeModel::Cut(SoBaseKit* parent, int row)
{
    if (row < 0) return false;

    QList<InstanceNode*> instancesParent = m_mapCoinQt[parent];
    InstanceNode* instanceParent = instancesParent[0]; //?
    QModelIndex indexParent = createIndex(instanceParent->getParent()->children.indexOf(instanceParent), 0, instanceParent);
    beginRemoveRows(indexParent, row, row);

    SoNode* node = instanceParent->children[row]->getNode();
    if (!node->getTypeId().isDerivedFrom(SoBaseKit::getClassTypeId()))
    {
//        SbString partName = parent->getPartString(node);
//        if (partName.getLength() == 0) partName = "material"; //?
//        parent->setPart(partName, 0);
    }
    else
    {
        if (parent->getTypeId().isDerivedFrom(TSeparatorKit::getClassTypeId()))
        {
            SoGroup* childList = (SoGroup*) parent->getPart("group", false);
            if (!childList) return false;

            int r = row;
            node = childList->getChild(r);
            childList->removeChild(r);

            if (!node) gcf::SevereError("SceneModel::Cut Null coinChild pointer");
        }
    }

    QList<InstanceNode*> instances = m_mapCoinQt[node];

    // Test if the node is shared
    if (instances.size() == 1) //?
    {
        InstanceNode* instance = instances[0];
        deleteInstanceTree(instance);
        //delete instanceNode; //?
    }
    else
    {
        for (InstanceNode* instanceParent : m_mapCoinQt[parent])
        {
            InstanceNode* instance = instanceParent->children[row];
            deleteInstanceTree(instance);
            //delete instanceNode;
        }
    }

    endRemoveRows();
    emit layoutChanged();

    return true;
}

bool SceneTreeModel::Paste(SoBaseKit* parent, SoNode* node, int row, bool isShared) // bugs
{
    SoNode* child;
    if (isShared)
        child = node;
    else
        child = node->copy(true);

    if (child->getTypeId().isDerivedFrom(SoBaseKit::getClassTypeId()))
    { // for TSeparatorKit, TrackerKit, TShapeKit
        SoGroup* parts = (SoGroup*) parent->getPart("group", true);
        parts->insertChild(child, row);
    }

    for (InstanceNode* instanceParent : m_mapCoinQt[parent])
    {
        InstanceNode* instance = new InstanceNode(child);
        instanceParent->insertChild(row, instance);
        m_mapCoinQt[child].append(instance);
        m_instances.insert(instance);
        generateInstanceTree(instance);
    }

    emit layoutChanged();
    return true;
}

void SceneTreeModel::UpdateSceneModel()
{
    emit layoutChanged();
}
