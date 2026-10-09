#include "RayTracingDialog.h"
#include "ui_RayTracingDialog.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QRegularExpressionValidator>

#include "run/GuiTraceSeed.h"
#include "kernel/photons/PhotonsAbstract.h"
#include "kernel/photons/PhotonsWidget.h"
#include "kernel/photons/PhotonsSettings.h"
#include "SelectSurfaceDialog.h"

/*!
 * Creates a new dialog object to define the export settings. The available export mode types are listed into \a typeList.
 */
RayTracingDialog::RayTracingDialog(QWidget* parent):
    QDialog(parent),
    ui(new Ui::RayTracingDialog)
{
    ui->setupUi(this);

    ui->fixedSeedEdit->setValidator(new QRegularExpressionValidator(
        QRegularExpression(QStringLiteral("[0-9]{1,10}")), ui->fixedSeedEdit));
    connect(ui->fixedSeedCheck, &QCheckBox::toggled,
            ui->fixedSeedEdit, &QWidget::setEnabled);
    connect(ui->fixedSeedCheck, &QCheckBox::toggled,
            this, &RayTracingDialog::updateFixedSeedValidity);
    connect(ui->fixedSeedEdit, &QLineEdit::textChanged,
            this, &RayTracingDialog::updateFixedSeedValidity);
    updateFixedSeedValidity();

    connect(ui->storeTypeCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(outputChanged()) );
    connect(ui->surfaceAddButton, SIGNAL(clicked()), this, SLOT(surfaceAdd()) );
    connect(ui->surfaceDeleteButton, SIGNAL(clicked()), this, SLOT(surfaceDelete()) );
}

RayTracingDialog::~RayTracingDialog()
{
    delete ui;
}

void RayTracingDialog::setParameters(int raysNumber, int raysScreen, int raysGridWidth, int raysGridHeight, int photonBufferSize, bool photonBufferAppend)
{
    ui->raysNumberSpin->setValue(raysNumber);
    ui->raysScreenSpin->setValue(raysScreen);
    ui->raysPlaneWidthSpin->setValue(raysGridWidth);
    ui->raysPlaneHeightSpin->setValue(raysGridHeight);

    ui->photonBufferSizeSpin->setValue(photonBufferSize);
    ui->photonBufferAppendRadio->setChecked(photonBufferAppend);
}

int RayTracingDialog::raysNumber() const {return ui->raysNumberSpin->value();}
int RayTracingDialog::raysScreen() const {return ui->raysScreenSpin->value();}
int RayTracingDialog::raysGridWidth() const {return ui->raysPlaneWidthSpin->value();}
int RayTracingDialog::raysGridHeight() const {return ui->raysPlaneHeightSpin->value();}

void RayTracingDialog::setFixedSeed(const std::optional<std::uint64_t>& seed)
{
    ui->fixedSeedEdit->setText(seed
        ? QString::number(static_cast<qulonglong>(*seed))
        : QStringLiteral("123456789"));
    ui->fixedSeedCheck->setChecked(seed.has_value());
    ui->fixedSeedEdit->setEnabled(seed.has_value());
    updateFixedSeedValidity();
}

std::optional<std::uint64_t> RayTracingDialog::fixedSeed() const
{
    if (!ui->fixedSeedCheck->isChecked())
        return std::nullopt;
    std::uint64_t seed = 0;
    if (!GuiTraceSeed::parseFixedSeed(ui->fixedSeedEdit->text(), &seed))
        return std::nullopt;
    return seed;
}

void RayTracingDialog::updateFixedSeedValidity()
{
    std::uint64_t seed = 0;
    const bool valid = !ui->fixedSeedCheck->isChecked()
        || GuiTraceSeed::parseFixedSeed(ui->fixedSeedEdit->text(), &seed);
    if (QPushButton* ok = ui->buttonBox->button(QDialogButtonBox::Ok))
        ok->setEnabled(valid);
}

int RayTracingDialog::photonBufferSize() const {return ui->photonBufferSizeSpin->value();}
bool RayTracingDialog::photonBufferAppend() const {return ui->photonBufferAppendRadio->isChecked();}


void RayTracingDialog::setPhotonSettings(SceneTreeModel* scene, QVector<PhotonsFactory*> factories, PhotonsSettings* ps)
{
    m_scene = scene;
    for (PhotonsFactory* f : factories)
    {
        ui->storeTypeCombo->addItem(f->icon(), f->name());
        PhotonsWidget* widget = f->createWidget();
        ui->parametersWidget->addWidget(widget);
    }

    if (!ps) return;
    ui->storeTypeCombo->setCurrentText(ps->name);
    outputChanged();

    ui->saveCoordinatesCheck->setChecked(ps->saveCoordinates);
    ui->saveGlobalRadio->setChecked(ps->saveCoordinatesGlobal);
    ui->saveSurfaceCheck->setChecked(ps->saveSurfaceID);
    ui->saveSideCheck->setChecked(ps->saveSurfaceSide);
    ui->savePhotonsCheck->setChecked(ps->savePhotonsID);

    ui->surfaceAllRadio->setChecked(ps->surfaces.isEmpty());

}

/*!
 *    Returns defined settings into settings class object.
 */
PhotonsSettings RayTracingDialog::getPhotonSettings() const
{
    PhotonsSettings settings;
    settings.name = ui->storeTypeCombo->currentText();

    settings.saveCoordinates = ui->saveCoordinatesCheck->isEnabled() && ui->saveCoordinatesCheck->isChecked();
    settings.saveCoordinatesGlobal = ui->saveGlobalRadio->isEnabled() && ui->saveGlobalRadio->isChecked();
    settings.saveSurfaceID = ui->saveSurfaceCheck->isEnabled() && ui->saveSurfaceCheck->isChecked();
    settings.saveSurfaceSide = ui->saveSideCheck->isEnabled() && ui->saveSideCheck->isChecked();
    settings.savePhotonsID = ui->savePhotonsCheck->isEnabled() && ui->savePhotonsCheck->isChecked();

    if (ui->surfaceSelectedRadio->isChecked())
        settings.surfaces = m_surfaces;

    PhotonsWidget* widget = (PhotonsWidget*) ui->parametersWidget->currentWidget();
    for (QString name : widget->getParameterNames())
        settings.parameters.insert(name, widget->getParameterValue(name));

    return settings;
}

/*!
 * Changes the widget of the store type settings to current selected store widget.
 */
void RayTracingDialog::outputChanged()
{
    ui->parametersWidget->setCurrentIndex(ui->storeTypeCombo->currentIndex());
}

/*!
 * Adds a surface to export surface list.
 */
void RayTracingDialog::surfaceAdd()
{
    SelectSurfaceDialog selectSurfaceDialog(*m_scene, true);
    if (!selectSurfaceDialog.exec()) return;

    QString surface = selectSurfaceDialog.GetSelectedSurfaceURL();
    if (surface.isEmpty()) return;

    if (m_surfaces.contains(surface)) {
        QMessageBox::information(this, "Tonatiuh", "Selected node has already been added");
        return;
    }

    m_surfaces << surface;
    ui->surfacesListWidget->addItem(surface);
}

/*!
 * Deletes current selected surface from export surface list.
 */
void RayTracingDialog::surfaceDelete()
{
    if (!ui->surfacesListWidget->currentItem()) return;
    int n = m_surfaces.indexOf(ui->surfacesListWidget->currentItem()->text());
    if (n < 0) return;
    m_surfaces.removeAt(n);
    delete ui->surfacesListWidget->item(n);
}
