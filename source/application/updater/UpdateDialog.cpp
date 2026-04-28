#include "UpdateDialog.h"
#include "ui_UpdateDialog.h"

#include "IfwUpdateService.h"

#include <QApplication>

UpdateDialog::UpdateDialog(IfwUpdateService* updateService, QWidget* parent):
    QDialog(parent),
    ui(new Ui::UpdateDialog),
    m_updateService(updateService),
    m_updateRequested(false)
{
    ui->setupUi(this);
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);

    ui->downloadButton->setText("Install Updates");
    ui->downloadButton->setEnabled(false);

    if (m_updateService) {
        connect(m_updateService, &IfwUpdateService::statusChanged, this, [this](IfwUpdateService::Status) {
            refresh();
        });
    }

    refresh();
    ui->checkButton->setFocus();
}

UpdateDialog::~UpdateDialog()
{
    delete ui;
}

void UpdateDialog::checkUpdates()
{
    if (!m_updateService) {
        showMessage("Update checks are not available.");
        return;
    }

    m_updateService->checkForUpdates();
    refresh();
}

void UpdateDialog::on_checkButton_pressed()
{
    checkUpdates();
}

void UpdateDialog::on_downloadButton_pressed()
{
    if (!m_updateService || !m_updateService->updateAvailable())
        return;

    m_updateRequested = true;
    accept();
}

void UpdateDialog::refresh()
{
    if (!m_updateService) {
        showMessage("Update checks are not available.");
        ui->checkButton->setEnabled(false);
        ui->downloadButton->setEnabled(false);
        return;
    }

    QString message = m_updateService->statusMessage();
    QString details = m_updateService->updateDetails();
    if (!details.isEmpty())
        message += QString("\n\n%1").arg(details);

    QString toolPath = m_updateService->maintenanceToolPath();
    if (!toolPath.isEmpty())
        message += QString("\n\nMaintenanceTool: %1").arg(toolPath);

    if (m_updateService->status() == IfwUpdateService::Idle)
        message = QString("Installed version: %1\n\nPress Check to ask the MaintenanceTool for available updates.")
            .arg(qApp->applicationVersion());
    else if (m_updateService->status() == IfwUpdateService::UpdateAvailable)
        message += "\n\nClick Install Updates to start the MaintenanceTool. Tonatiuh++ will close before the updater starts. Please restart Tonatiuh++ manually after the update completes.";

    showMessage(message);
    ui->checkButton->setEnabled(!m_updateService->isChecking());
    ui->downloadButton->setEnabled(m_updateService->updateAvailable() && !m_updateService->isChecking());
}

void UpdateDialog::showMessage(const QString& message)
{
    ui->resultText->setPlainText(message);
}
