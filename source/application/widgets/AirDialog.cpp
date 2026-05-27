#include "AirDialog.h"
#include "ui_AirDialog.h"

#include <Inventor/fields/SoField.h>

#include "kernel/scene/TSceneKit.h"
#include "kernel/air/AirKit.h"
#include "kernel/air/AirTransmission.h"


AirDialog::AirDialog(TSceneKit* sceneKit, QWidget* parent):
    QDialog(parent),
    ui(new Ui::AirDialog)
{
    ui->setupUi(this);

    int q = fontMetrics().height();
    resize(64*q, 36*q);

    int w = width();
    ui->splitter->setSizes({ int(0.4*w), int(0.6*w) });

    AirKit* air = (AirKit*) sceneKit->getPart("world.air", false);
    m_air = (AirKit*) air->copy();
    m_air->ref();

    initCustomPlot();
    updateCustomPlot();

    ui->airParameters->getModel()->setMain((MainWindow*) parent);
    ui->airParameters->setNode(m_air);

    connect(
        ui->airParameters->getModel(), SIGNAL(fieldTextModified(SoNode*, QString, QString)),
        this, SLOT(setFieldText(SoNode*, QString, QString))
    );
    connect(
        ui->airParameters->getModel(), SIGNAL(fieldNodeModified(SoNode*, QString, SoNode*)),
        this, SLOT(setFieldNode(SoNode*, QString, SoNode*))
    );

    setWindowFlag(Qt::WindowContextHelpButtonHint, false);
}

AirDialog::~AirDialog()
{
    delete ui;
    m_air->unref();
}

void AirDialog::setFieldText(SoNode* node, QString field, QString value)
{
    SoField* f = node->getField(field.toLatin1().data());
    f->set(value.toLatin1().data());
    updateCustomPlot();
}

void AirDialog::setFieldNode(SoNode* node, QString field, SoNode* value)
{
    SoSFNode* f = (SoSFNode*) node->getField(field.toLatin1().data());
    f->setValue(value);
    updateCustomPlot();
}

void AirDialog::initCustomPlot()
{
    QCustomPlot* cp = ui->customPlot;
    QFont cfont = font();
    cfont.setPointSize(9);
    cp->setFont(cfont);

    // axes
    cp->addGraph();
    cp->xAxis->setLabelFont(cfont);
    cp->xAxis->setLabel("Distance, d [m]");
    cp->yAxis->setLabelFont(cfont);
    cp->yAxis->setLabel("Transmission, t [%]");

    cp->xAxis2->setVisible(true);
    cp->xAxis2->setTickLabels(false);
    connect(cp->xAxis, SIGNAL(rangeChanged(QCPRange)), cp->xAxis2, SLOT(setRange(QCPRange)));

    cp->yAxis2->setVisible(true);
    cp->yAxis2->setTickLabels(false);
    connect(cp->yAxis, SIGNAL(rangeChanged(QCPRange)), cp->yAxis2, SLOT(setRange(QCPRange)));

    cp->xAxis->setRange(0., 2000.);
    cp->yAxis->setRange(0., 105.);

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
    cp->plotLayout()->addElement(0, 0, new QCPTextElement(cp, "Atmospheric transmission", cfont));

    cp->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
}

void AirDialog::updateCustomPlot()
{
    QCustomPlot* cp = ui->customPlot;
    if (!m_air) return;
    AirTransmission* airTransmission = (AirTransmission*) m_air->getPart("transmission", false);

    int nMax = 51;
    double xStep = 2000./(nMax - 1);
    QVector<double> x(nMax), y(nMax);
    for (int n = 0; n < nMax; ++n) {
        double d = n*xStep;
        x[n] = d;
        y[n] = 100.*airTransmission->transmission(d);
    }

    cp->graph(0)->setData(x, y);
    cp->replot();
}
