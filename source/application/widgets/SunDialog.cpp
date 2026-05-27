#include "SunDialog.h"
#include "ui_SunDialog.h"

#include "tree/SceneTreeModel.h"
#include "kernel/run/InstanceNode.h"
#include "kernel/scene/TShapeKit.h"
#include "kernel/shape/ShapeRT.h"
#include "kernel/sun/SunShape.h"
#include "kernel/sun/SunPosition.h"
#include "kernel/sun/SunAperture.h"
#include "kernel/sun/SunKit.h"
#include "kernel/scene/TSceneKit.h"
#include "calculator/SunCalculatorDialog.h"
#include "parameters/ParametersModel.h"


SunDialog::SunDialog(SceneTreeModel* sceneModel, QWidget* parent):
    QDialog(parent),
    ui(new Ui::SunDialog),
    m_sceneModel(sceneModel)
{
    ui->setupUi(this);

    int q = fontMetrics().height();
    resize(64*q, 36*q);

    int w = width();
    ui->splitter->setSizes({ int(0.4*w), int(0.6*w) });

    TSceneKit* sceneKit = sceneModel->getSceneKit();
    SunKit* sun = (SunKit*) sceneKit->getPart("world.sun", false);
    m_sun = (SunKit*) sun->copy();
    m_sun->ref();

    ui->sunParameters->getModel()->setMain((MainWindow*) parent);
    ui->sunParameters->setNode(m_sun);

    connect(
        ui->sunParameters, SIGNAL(pressed(QModelIndex)),
        this, SLOT(onPressed(QModelIndex))
    );
    connect(
        ui->sunParameters->getModel(), SIGNAL(fieldTextModified(SoNode*, QString, QString)),
        this, SLOT(setFieldText(SoNode*, QString, QString))
    );
    connect(
        ui->sunParameters->getModel(), SIGNAL(fieldNodeModified(SoNode*, QString, SoNode*)),
        this, SLOT(setFieldNode(SoNode*, QString, SoNode*))
    );

    initCustomPlot();
    updateCustomPlot();

    makeSunPositionTab();
    makeSunShapeTab();
    makeSunApertureTab();

    setWindowFlag(Qt::WindowContextHelpButtonHint, false);
}

SunDialog::~SunDialog()
{
    delete ui;
    m_sun->unref();
}

void SunDialog::on_buttonSunCalc_clicked()
{
    SunCalculatorDialog dialog(this);
    connect(
        &dialog, SIGNAL(changeSunLight(double, double)),
        this, SLOT(setAzEl(double, double))
    );
    dialog.exec();
}

void SunDialog::setFieldText(SoNode* node, QString field, QString value)
{
    if (field == "irradiance")
        if (value.toDouble() < 0.) return;

    SoField* f = node->getField(field.toLatin1().data());
    f->set(value.toLatin1().data());

    updateCustomPlot(); // not always
}

void SunDialog::setFieldNode(SoNode* node, QString field, SoNode* value)
{
    SoSFNode* f = (SoSFNode*) node->getField(field.toLatin1().data());
    f->setValue(value);
    updateCustomPlot();
}

void SunDialog::setAzEl(double az, double el)
{
    SunPosition* sp = (SunPosition*) m_sun->getPart("position", false);
    setFieldText(sp, "azimuth", QString::number(az));
    setFieldText(sp, "elevation", QString::number(el));
}

#include <QDebug>
void SunDialog::onPressed(const QModelIndex& index)
{
    QStandardItem* item = ui->sunParameters->getModel()->itemFromIndex(index);
    int row = 0;
    while (item->parent()) {
        row = item->row();
        item = item->parent();
    }

   ui->stackedWidgets->setCurrentIndex(row);
}

void SunDialog::makeSunPositionTab()
{

}

void SunDialog::makeSunShapeTab()
{

}

void SunDialog::makeSunApertureTab()
{
    ui->modelTreeView->setModel(m_sceneModel);
    QItemSelectionModel* selectionModel = new QItemSelectionModel(m_sceneModel);
    ui->modelTreeView->setSelectionModel(selectionModel);
    ui->modelTreeView->setRootIndex(QModelIndex());

    SunAperture* aperture = (SunAperture*) m_sun->getPart("aperture", false);
    QString nodes = aperture->disabledNodes.getValue().getString();
    for (QString s : nodes.split(';', Qt::SkipEmptyParts))
        ui->disabledNodes->addItem(s);

    connect(
        ui->addNodeButton, SIGNAL(clicked()),
        this, SLOT(addNode())
    );
    connect(
        ui->removeNodeButton, SIGNAL(clicked()),
        this, SLOT(removeNode())
    );
}

void SunDialog::addNode()
{
    QItemSelectionModel* selectionModel = ui->modelTreeView->selectionModel();
    if (!selectionModel->hasSelection()) return;
    QModelIndex index = selectionModel->currentIndex();
    if (index == ui->modelTreeView->rootIndex()) return;
    if (!index.isValid()) return;
    InstanceNode* node = m_sceneModel->getInstance(index);
    if (!node) return;

    ui->disabledNodes->addItem(node->getURL());
    updateAperture();
}

void SunDialog::removeNode()
{
    QListWidgetItem* item = ui->disabledNodes->currentItem();
    if (!item) return;

    ui->disabledNodes->removeItemWidget(item);
    delete item;
    updateAperture();
}

void SunDialog::updateAperture()
{
    SunAperture* aperture = (SunAperture*) m_sun->getPart("aperture", false);
    QStringList nodes;
    for (int n = 0; n < ui->disabledNodes->count(); ++n)
        nodes << ui->disabledNodes->item(n)->text();
    aperture->disabledNodes = nodes.join(';').toStdString().c_str();

    //    m_sun->updateTransform();
}

void SunDialog::initCustomPlot()
{
    QCustomPlot* cp = ui->customPlot;
    QFont cfont = font();
    cfont.setPointSize(9);
    cp->setFont(cfont);

    // axes
    cp->addGraph();
    cp->xAxis->setLabelFont(cfont);
    cp->xAxis->setLabel("Angle, alpha [mrad]");
    cp->yAxis->setLabelFont(cfont);
    cp->yAxis->setLabel("Radiance, L [au]");

    cp->xAxis2->setVisible(true);
    cp->xAxis2->setTickLabels(false);
    connect(cp->xAxis, SIGNAL(rangeChanged(QCPRange)), cp->xAxis2, SLOT(setRange(QCPRange)));

    cp->yAxis2->setVisible(true);
    cp->yAxis2->setTickLabels(false);
    connect(cp->yAxis, SIGNAL(rangeChanged(QCPRange)), cp->yAxis2, SLOT(setRange(QCPRange)));

    cp->xAxis->setRange(0., 8.);
    cp->yAxis->setRange(0., 1.1);

    // line
    QPen pen("#225F8E");
    pen.setWidthF(2);
//    pen.setStyle(Qt::DashLine);
    cp->graph(0)->setPen(pen);
//    cp->graph(0)->setLineStyle(QCPGraph::lsLine);
//    cp->graph(0)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDisc, 5));
    cp->graph(0)->setBrush(QBrush("#30225F8E"));

    // label
    cp->plotLayout()->insertRow(0);
    cfont.setPointSize(10);
//    cfont.setBold(true);
    cp->plotLayout()->addElement(0, 0, new QCPTextElement(cp, "Sun shape", cfont));

    cp->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
}

void SunDialog::updateCustomPlot()
{
    QCustomPlot* cp = ui->customPlot;
    if (!m_sun) return;
    SunShape* shape = (SunShape*) m_sun->getPart("shape", false);

    int nMax = 101;
    double xStep = 8./(nMax - 1);
    QVector<double> x(nMax), y(nMax);
    for (int n = 0; n < nMax; ++n) {
        double theta = n*xStep;
        x[n] = theta;
        y[n] = shape->shape(0.001*theta);
    }

    cp->graph(0)->setData(x, y);
    cp->replot();
}
