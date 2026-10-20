#include "FluxAnalysisDialog.h"
#include "ui_FluxAnalysisDialog.h"

#include <cmath>

#include <QFileDialog>
#include <QFutureWatcher>
#include <QIntValidator>
#include <QMessageBox>
#include <QMutex>
#include <QPair>
#include <QProgressDialog>
#include <QtConcurrentMap>

#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/nodes/SoTransform.h>

#include "FluxAnalysis.h"
#include "SelectSurfaceDialog.h"
#include "tree/SceneTreeModel.h"
#include "kernel/air/AirTransmission.h"
#include "kernel/run/InstanceNode.h"
#include "kernel/random/Random.h"
#include "kernel/run/RayTracer.h"
#include "kernel/scene/TSceneKit.h"
#include "kernel/scene/TShapeKit.h"
#include "kernel/shape/ShapeRT.h"
#include "kernel/sun/SunKit.h"
#include "kernel/sun/SunAperture.h"
#include "libraries/math/3D/Transform.h"
#include "libraries/math/gcf.h"



FluxAnalysisDialog::FluxAnalysisDialog(TSceneKit* sceneKit, SceneTreeModel* sceneModel,
                                       int sunWidthDivisions, int sunHeightDivisions,
                                       std::uint64_t masterSeed, QWidget* parent):
    QDialog(parent),
    ui(new Ui::FluxAnalysisDialog),
    m_sceneModel(sceneModel),
    m_fluxLabel("Flux, W/m^2")
{
    ui->setupUi(this);

    m_fluxAnalysis = new FluxAnalysis(sceneKit, sceneModel, sunWidthDivisions, sunHeightDivisions, masterSeed);

    connect(ui->surfaceButton, SIGNAL(clicked()), this, SLOT(SurfaceSelected()));
    connect(ui->surfaceEdit, SIGNAL(editingFinished()), this, SLOT(SurfaceChanged()));
    connect(ui->surfaceSideCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(SideChanged()));
    connect(ui->surfaceXSpin, SIGNAL(editingFinished()), this, SLOT(UpdateAnalysis()));
    connect(ui->surfaceYSpin, SIGNAL(editingFinished()), this, SLOT(UpdateAnalysis()));
    connect(ui->raysButton, SIGNAL(clicked()), this, SLOT(run()));

    connect(ui->exportLengthEdit, SIGNAL(editingFinished()), this, SLOT(UnitsChanged()));
    connect(ui->exportPowerEdit, SIGNAL(editingFinished()), this, SLOT(UnitsChanged()));

    connect(ui->spinX, SIGNAL(valueChanged(double)), this, SLOT(UpdateSectorPlotSlot()));
    connect(ui->spinY, SIGNAL(valueChanged(double)), this, SLOT(UpdateSectorPlotSlot()));

    ui->plotFxy->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom); // this will also allow rescaling the color scale by dragging/zooming
    ui->plotFxy->axisRect()->setupFullAxesBox(true);
    ui->plotFxy->plotLayout()->insertRow(0);
    ui->plotFxy->plotLayout()->addElement(0, 0, new QCPTextElement(ui->plotFxy, "Flux Distribution") );
    ui->plotFxy->xAxis->setLabel("u");
    ui->plotFxy->yAxis->setLabel("v");

    ui->plotFx->xAxis->setLabel("u");
    ui->plotFx->yAxis->setLabel(m_fluxLabel);
    ui->plotFx->yAxis->setRange(0, 1.08);

    ui->plotFy->xAxis->setLabel("v");
    ui->plotFy->yAxis->setLabel(m_fluxLabel);
    ui->plotFy->yAxis->setRange(0, 1.08);

    int q = fontMetrics().height();
    resize(64*q, 48*q);

    setWindowFlag(Qt::WindowMinimizeButtonHint, false);
    setWindowFlag(Qt::WindowMaximizeButtonHint, true);
}

FluxAnalysisDialog::~FluxAnalysisDialog()
{
    delete m_fluxAnalysis;
    delete ui;
}

QStringList sFilters = {"Planar", "Parabolic", "Elliptic", "Hyperbolic", "Cylinder", "Sphere"};
//#include <QSet>
//QSet<QString> setFilters = {"Planar", "Parabolic", "Elliptic", "Hyperbolic", "Cylinder"};

/*!
 * Opens a dialog to select a surface from the system.
 * If the selected surface is ok
 */
void FluxAnalysisDialog::SurfaceSelected()
{
    SelectSurfaceDialog dialog(*m_sceneModel, false, this);
    dialog.SetShapeTypeFilters(sFilters);
    if (!dialog.exec()) return;

    QString fluxSurfaceURL = dialog.GetSelectedSurfaceURL(); //bug
    if (fluxSurfaceURL.isEmpty()) return;
    if (fluxSurfaceURL == m_fluxSurfaceURL) return;

    m_fluxAnalysis->clear();
    ClearAnalysis();
    ui->surfaceEdit->setText(fluxSurfaceURL);
    m_fluxSurfaceURL = fluxSurfaceURL;
}

/*!
 * Changes current surface to the surface node defined in surface line.
 */
void FluxAnalysisDialog::SurfaceChanged()
{
    QString fluxSurfaceURL = ui->surfaceEdit->text();
    if (fluxSurfaceURL.isEmpty() || fluxSurfaceURL == m_fluxSurfaceURL)
    {
        ui->surfaceEdit->setText(m_fluxSurfaceURL);
        return;
    }

    QString shapeType = m_fluxAnalysis->getShapeType(fluxSurfaceURL);
    if (sFilters.contains(shapeType))
    {
        m_fluxAnalysis->clear();
        ui->raysAppendCheck->setChecked(false);
        ui->raysAppendCheck->setEnabled(false);
        ClearAnalysis();
        ui->surfaceEdit->setText(fluxSurfaceURL);
        m_fluxSurfaceURL = fluxSurfaceURL;
    }
}

/*
 * Clear the current analysis if the surface side is modified
 */
void FluxAnalysisDialog::SideChanged()
{
    m_fluxAnalysis->clear();
    ui->raysAppendCheck->setChecked(false);
    ui->raysAppendCheck->setEnabled(false);
    ClearAnalysis();
}

/*
 * Calculate flux distribution and statistics
 */
void FluxAnalysisDialog::UpdateAnalysis()
{
//	int pos = 0;
//    QString withValue = ui->spinCellsX->text();
//    QValidator::State widthState = m_pGridWidthVal->validate(withValue, pos);

//    QString heightValue = ui->surfaceYSpin->text();
//    QValidator::State heightSate = m_pGridHeightVal->validate(heightValue, pos);

//    if ( (widthState != QValidator::Acceptable) || (heightSate != QValidator::Acceptable) )
//	{
//        QMessageBox::warning(this, "Tonatiuh",
//                             tr("The gird divisions size must be greater than 2.") );
//		return;
//	}

    if (m_fluxAnalysis->getBinsPhotons().isEmpty())
        return;

    vec2i divs(ui->surfaceXSpin->value(), ui->surfaceYSpin->value());
    m_fluxAnalysis->setBins(divs.x, divs.y);

//    const Matrix2D<int>& photonCounts = m_fluxAnalysis->getBinsPhotons();
    const Matrix2D<double>& fluxCounts = m_fluxAnalysis->getBinsFlux();

    ClearAnalysis();

    Box2D box = m_fluxAnalysis->box();
    double uMin = box.min().x;
    double vMin = box.min().y;
    double uMax = box.max().x;
    double vMax = box.max().y;
    double powerPhoton = m_fluxAnalysis->powerPhoton();
    double powerTotal = m_fluxAnalysis->powerTotal();

    double uStep = box.size().x/divs.x;
    double vStep = box.size().y/divs.y;
    double areaCell = findAreaCell(uStep, vStep);

    double fluxMin = 0.;
    double fluxAverage = powerTotal/(areaCell*divs.x*divs.y); // wrong
    double fluxMax = m_fluxAnalysis->photonsMax()*powerPhoton/areaCell;
    double fluxMaxU = uMin + (m_fluxAnalysis->getPhotonMaxPos().x + 0.5)*uStep;
    double fluxMaxV = vMin + (m_fluxAnalysis->getPhotonMaxPos().y + 0.5)*vStep;

    double areaE = box.size().x/(divs.x - 1)*box.size().y/(divs.y - 1);
    double maximumFluxError = m_fluxAnalysis->photonsError()*powerPhoton/areaE;
    double error = fabs(fluxMax - maximumFluxError)/fluxMax;
    double fluxMeanU = 0.;
    double fluxMeanV = 0.;
    double E = 0;

    for (int r = 0; r < divs.x; ++r)
    {
        for (int c = 0; c < divs.y; ++c)
        {
            double fluxCell = fluxCounts(r, c);
            if (fluxMin > fluxCell) fluxMin = fluxCell;

            fluxMeanU += fluxCell*(uMin + (r + 0.5)*uStep);
            fluxMeanV += fluxCell*(vMin + (c + 0.5)*vStep);
            E += gcf::pow2(fluxCell - fluxAverage);
        }
    }

    double standardDeviation = sqrt(E/(divs.x*divs.y));
    double uniformity = standardDeviation/fluxAverage;
    double fluxNorm = powerTotal/areaCell;
    fluxMeanU /= fluxNorm;
    fluxMeanV /= fluxNorm;

    UpdateStatistics(powerTotal, fluxMin, fluxAverage, fluxMax, fluxMaxU, fluxMaxV, error, uniformity, fluxMeanU, fluxMeanV);
    UpdateFluxMapPlot(fluxCounts, box);
    CreateSectorPlots(uMin, vMin, uMax, vMax);
    UpdateSectorPlots(fluxCounts, box);
}

/*!
 * Runs flux analysis for current defined surface.
 */
void FluxAnalysisDialog::run()
{
    QElapsedTimer timer;
    timer.start();

    if (m_fluxSurfaceURL.isEmpty())
    {
        QMessageBox::warning(this, "Tonatiuh", "Select a valid surface.");
        return;
    }
    QString surfaceSide = ui->surfaceSideCombo->currentText();
    bool increasePhotonMap = ui->raysAppendCheck->isEnabled() && ui->raysAppendCheck->isChecked();

    if (!m_fluxAnalysis->run(m_fluxSurfaceURL, surfaceSide, ui->raysSpin->value(), increasePhotonMap, ui->surfaceXSpin->value(), ui->surfaceYSpin->value())) {
        ui->raysAppendCheck->setChecked(false);
        ui->raysAppendCheck->setEnabled(false);
        ClearAnalysis();
        return;
    }
    UpdateAnalysis();
    ui->raysAppendCheck->setEnabled(true);

    std::cout << "Elapsed time: " << timer.elapsed() << std::endl;
}

/*
 * Updates the labels of the plots
 */
void FluxAnalysisDialog::UnitsChanged()
{
    QString lengthUnit = ui->exportLengthEdit->text();
    if (lengthUnit.isEmpty()) lengthUnit = "m";

    QString powerUnit = ui->exportPowerEdit->text();
    if (powerUnit.isEmpty()) powerUnit = "W";

    ui->plotFxy->xAxis->setLabel(QString("x, %1").arg(lengthUnit));
    ui->plotFxy->yAxis->setLabel(QString("y, %1").arg(lengthUnit));
    m_fluxLabel = QString("Flux, %1/%2^2").arg(powerUnit, lengthUnit);
    QCPColorMap* colorMapPlot = qobject_cast<QCPColorMap*>(ui->plotFxy->plottable());
    for (int i = 0; i < ui->plotFxy->plotLayout()->elementCount(); i++)
    {
        if (qobject_cast<QCPColorScale*>(ui->plotFxy->plotLayout()->elementAt(i) ) )
            colorMapPlot->colorScale()->axis()->setLabel(m_fluxLabel);
    }
    ui->plotFxy->replot();

    ui->plotFx->xAxis->setLabel(QString("x, %1").arg(lengthUnit));
    ui->plotFx->yAxis->setLabel(QString("Flux, %1/%2^2").arg(powerUnit, lengthUnit));
    ui->plotFx->replot();

    ui->plotFy->xAxis->setLabel(QString("y, %1").arg(lengthUnit));
    ui->plotFy->yAxis->setLabel(QString("Flux, %1/%2^2").arg(powerUnit, lengthUnit));
    ui->plotFy->replot();
}

void FluxAnalysisDialog::on_pushButton_clicked()
{
    QString dirName;
    QString selectedFilter;
    QSettings settings("Tonatiuh", "Cyprus");
    if (m_path.isEmpty()) {
        dirName = settings.value("dirProjects", "").toString();
    } else {
        dirName = m_path;
        QFileInfo info(m_path);
        if (info.suffix() == "jpg")
            selectedFilter = "Image (*.jpg)";
        else if (info.suffix() == "txt")
            selectedFilter = "Data (*.txt)";
        else if (info.suffix() == "dat")
            selectedFilter = "Data with grid (*.dat)";
    }

    QString fileName = QFileDialog::getSaveFileName(
        this, "Export", dirName,
        "Image (*.png);;Image (*.jpg);;Data (*.txt);;Data with grid (*.dat)",
         &selectedFilter
    );
    if (fileName.isEmpty()) return;
    settings.setValue("dirProjects", dirName);
    m_path = fileName;


    const Matrix2D<int>& photonCounts = m_fluxAnalysis->getBinsPhotons();
    if (photonCounts.data().isEmpty())
    {
        QString message = QString("Nothing available to export, first run the simulation");
        QMessageBox::warning(this, "Tonatiuh", message);
        return;
    }

    QFileInfo info(m_path);
    if (info.suffix() == "txt")
        m_fluxAnalysis->write(m_path, false);
    else if (info.suffix() == "dat")
        m_fluxAnalysis->write(m_path, true);
    else if (info.suffix() == "jpg")
        ui->plotFxy->saveJpg(m_path);
    else if (info.suffix() == "png")
        ui->plotFxy->savePng(m_path);
}

/*
 * Update plots when changing x or y coord
 */
void FluxAnalysisDialog::UpdateSectorPlotSlot()
{
    const Matrix2D<double>& flux = m_fluxAnalysis->getBinsFlux();
//    if (!photonCounts || photonCounts == 0) return;
    Box2D box = m_fluxAnalysis->box();
    UpdateSectorPlots(flux, box);
}

/*!
 * Clear current results and simulation.
 */
void FluxAnalysisDialog::ClearAnalysis()
{
    ui->plotFy->clearPlottables();
    ui->plotFy->yAxis->setRange(0, 1.05);
    ui->plotFy->replot();

    ui->plotFx->clearPlottables();
    ui->plotFx->yAxis->setRange(0, 1.05);
    ui->plotFx->replot();

    ui->plotFxy->clearPlottables();
    for (int i = 0; i < ui->plotFxy->plotLayout()->elementCount(); i++)
    {
        //test to see if any of the layout elements are of QCPColorScale type
        if (qobject_cast<QCPColorScale*>(ui->plotFxy->plotLayout()->elementAt(i)))
            ui->plotFxy->plotLayout()->removeAt(i);
        //collapse the empty elements
        ui->plotFxy->plotLayout()->simplify();
    }
    ui->plotFxy->replot();

//    ui->spinX->setValue(0.);
//    ui->spinX->setMinimum(0.);
//    ui->spinX->setMaximum(0.);
//    ui->spinY->setValue(0.);
//    ui->spinY->setMinimum(0.);
//    ui->spinY->setMaximum(0.);

    ui->powerTotal->setText(QString::number(0.));
    ui->fluxMin->setText(QString::number(0.));
    ui->fluxAverage->setText(QString::number(0.));
    ui->fluxMax->setText(QString::number(0.));

    ui->fluxMaxUV->setText(" ; ");
//    ui->errorValue->setText(QString::number(0.));
//    ui->uniformityValue->setText(QString::number(0.));
//    ui->centroidValue->setText(" ; ");
}

/*
 * Updates the statistics
 */
void FluxAnalysisDialog::UpdateStatistics(double powerTotal, double fluxMin, double fluxAverage, double fluxMax,
                                          double fluxMaxU, double fluxMaxV, double /*error*/, double /*uniformity*/, double /*gravityX*/, double /*gravityY*/)
{
    ui->powerTotal->setText(QString::number(powerTotal));
    ui->fluxMin->setText(QString::number(fluxMin));
    ui->fluxAverage->setText(QString::number(fluxAverage));
    ui->fluxMax->setText(QString::number(fluxMax));
    ui->fluxMaxUV->setText(QString("%1 ; %2").arg(QString::number(fluxMaxU), QString::number(fluxMaxV)));
//    ui->errorValue->setText(QString::number(error) );
//    ui->uniformityValue->setText(QString::number(uniformity) );
//    ui->centroidValue->setText(QString("%1 ; %2").arg(QString::number(gravityX), QString::number(gravityY)));
}

/*
 * Updates the flux map plot
 */
void FluxAnalysisDialog::UpdateFluxMapPlot(const Matrix2D<double>& flux, const Box2D& box)
{
    // delete previous colormap, scale
    ui->plotFxy->clearPlottables();
    ui->plotFxy->clearItems();

    for (int i = 0; i < ui->plotFxy->plotLayout()->elementCount(); i++)
	{
        if (qobject_cast<QCPColorScale*>(ui->plotFxy->plotLayout()->elementAt(i) ) )
            ui->plotFxy->plotLayout()->removeAt(i);
        // collapse the empty elements
        ui->plotFxy->plotLayout()->simplify();
	}

	// Create a QCPColorMap object to draw flux distribution
    QCPColorMap* colorMap = new QCPColorMap(ui->plotFxy->xAxis, ui->plotFxy->yAxis);
//    ui->plotFxy->addPlottable(colorMap);

    colorMap->data()->setSize(flux.rows(), flux.cols());
    colorMap->data()->setRange(QCPRange(box.min().x, box.max().x), QCPRange(box.min().y, box.max().y));
    for (int x = 0; x < flux.rows(); ++x)
        for (int y = 0; y < flux.cols(); ++y)
            colorMap->data()->setCell(x, y, flux(x, y));

	// add a color scale:
    QCPColorScale* colorScale = new QCPColorScale(ui->plotFxy);
    ui->plotFxy->plotLayout()->addElement(1, 1, colorScale); // add it to the right of the main axis rect
    colorScale->setType(QCPAxis::atRight); // scale shall be vertical bar with tick/axis labels right (actually atRight is already the default)
    colorScale->axis()->setLabel(m_fluxLabel);
    colorScale->setRangeZoom(false); // forbid to drag and zoom the colorscale
    colorScale->setRangeDrag(false);

    colorMap->setColorScale(colorScale);   // associate the color map with the color scale
    colorMap->setGradient(QCPColorGradient::gpThermal); // set the contour plot color
    colorMap->rescaleDataRange(); // rescale the data dimension (color) such that all data points lie in the span visualized by the color gradient:

	// make sure the axis rect and color scale synchronize their bottom and top margins (so they line up):
    QCPMarginGroup* marginGroup = new QCPMarginGroup(ui->plotFxy);
    ui->plotFxy->axisRect()->setMarginGroup(QCP::msBottom | QCP::msTop, marginGroup);
    colorScale->setMarginGroup(QCP::msBottom | QCP::msTop, marginGroup);

	// rescale axes and update plot view
    ui->plotFxy->rescaleAxes();
    ui->plotFxy->replot();
}

/*
 * Create sector plots
 */
void FluxAnalysisDialog::CreateSectorPlots(double xMin, double yMin, double xMax, double yMax)
{
    QPen pen(QColor(137, 140, 140), 1);

    m_lineV = new QCPItemLine(ui->plotFxy);
//    ui->plotFxy->addItem(tickVLine);
    m_lineV->start->setCoords(0, yMin - 1);
    m_lineV->end->setCoords(0, yMax + 1);
    m_lineV->setPen(pen);

    m_lineH = new QCPItemLine(ui->plotFxy);
//    ui->plotFxy->addItem(tickHLine);
    m_lineH->start->setCoords(xMin - 1,  0);
    m_lineH->end->setCoords(xMax + 1, 0);
    m_lineH->setPen(pen);

    // Create a vertical and horizontal line for sectors
    ui->spinX->setMinimum(xMin);
    ui->spinX->setMaximum(xMax);
    ui->spinX->setSingleStep((xMax - xMin)/10);

    ui->spinY->setMinimum(yMin);
    ui->spinY->setMaximum(yMax);
    ui->spinY->setSingleStep((yMax - yMin)/10);
}

/*
 * Updates the sector plots
 */
void FluxAnalysisDialog::UpdateSectorPlots(const Matrix2D<double>& flux, const Box2D& box)
{
    vec2i divs(flux.rows(), flux.cols());
    double x = ui->spinX->value();
    double y = ui->spinY->value();

//    m_lineV = (QCPItemLine*) ui->plotFxy->item(0);
    m_lineV->start->setCoords(x, m_lineV->start->coords().y());
    m_lineV->end->setCoords(x, m_lineV->end->coords().y());

//    m_lineH = (QCPItemLine*) ui->plotFxy->item(1);
    m_lineH->start->setCoords(m_lineH->start->coords().x(), y);
    m_lineH->end->setCoords(m_lineH->end->coords().x(), y);

    ui->plotFxy->replot(); // move lines
    ui->plotFx->clearPlottables();
    ui->plotFy->clearPlottables();


    double uStep = box.size().x/divs.x;
    double vStep = box.size().y/divs.y;

    int xBin = floor((x - box.min().x)/uStep);
    if (xBin >= divs.x) xBin = divs.x - 1;
    int yBin = floor((y - box.min().y)/vStep);
    if (yBin >= divs.y) yBin = divs.y - 1;

    double fMax = *std::max_element(flux.data().begin(), flux.data().end());
    QVector<double> dataY(divs.y), dataFy(divs.y);
    for (int ny = 0; ny < divs.y; ++ny)
	{
        dataY[ny] = box.min().y + (ny + 0.5)*vStep;
        dataFy[ny] = flux(xBin, ny);
	}

    QVector<double> dataX(divs.x), dataFx(divs.x);
    for (int nx = 0; nx < divs.x; ++nx)
	{
        dataX[nx] = box.min().x + (nx + 0.5)*uStep;
        dataFx[nx] = flux(nx, yBin);
	}

    ui->plotFy->addGraph();
    ui->plotFy->graph(0)->setData(dataY, dataFy);
    ui->plotFy->xAxis->setRange(box.min().y, box.max().y);
    ui->plotFy->yAxis->setRange(0, 1.2*fMax);
    ui->plotFy->replot();

    ui->plotFx->addGraph();
    ui->plotFx->graph(0)->setData(dataX, dataFx);
    ui->plotFx->xAxis->setRange(box.min().x, box.max().x);
    ui->plotFx->yAxis->setRange(0, 1.2*fMax);
    ui->plotFx->replot();
}

double FluxAnalysisDialog::findAreaCell(double uStep, double vStep)
{
    QModelIndex nodeIndex = m_sceneModel->indexFromUrl(m_fluxSurfaceURL);
    InstanceNode* instanceNode = m_sceneModel->getInstance(nodeIndex);
    vec3d sJ = instanceNode->getTransform().getScales();
    TShapeKit* kit = (TShapeKit*) instanceNode->getNode();
    ShapeRT* shape = (ShapeRT*) kit->shapeRT.getValue();
    // works for cylinder, not sphere
    double areaCell = dot(sJ, shape->getDerivativeU(0., 0.))*uStep;
    areaCell *= dot(sJ, shape->getDerivativeV(0., 0.))*vStep;

    return areaCell;
}
