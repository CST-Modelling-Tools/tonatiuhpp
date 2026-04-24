#include "UpdateDialog.h"
#include "ui_UpdateDialog.h"

#include "UpdateReader.h"

#include <QApplication>
#include <QDesktopServices>
#include <QMessageBox>
#include <QNetworkReply>
#include <QNetworkRequest>

namespace
{
const QUrl kLatestReleaseUrl("https://api.github.com/repos/CST-Modelling-Tools/tonatiuhpp/releases/latest");
}

UpdateDialog::UpdateDialog(QWidget* parent):
    QDialog(parent),
    ui(new Ui::UpdateDialog),
    m_reply(nullptr)
{
    ui->setupUi(this);

    setWindowFlag(Qt::WindowContextHelpButtonHint, false);

    ui->openReleaseButton->setEnabled(false);
    showResult(QString("Installed version: %1").arg(qApp->applicationVersion()));

    ui->checkButton->setFocus();
}

UpdateDialog::~UpdateDialog()
{
    if (m_reply) {
        disconnect(m_reply, nullptr, this, nullptr);
        m_reply->abort();
        m_reply->deleteLater();
    }

    delete ui;
}

void UpdateDialog::checkUpdates()
{
    if (m_reply)
        return;

    m_releaseUrl = QUrl();
    setChecking(true);
    showResult(QString("Checking for updates...\nInstalled version: %1").arg(qApp->applicationVersion()));

    QNetworkRequest request(kLatestReleaseUrl);
    request.setRawHeader("Accept", "application/vnd.github+json");
    request.setRawHeader("User-Agent", "TonatiuhPP");
    request.setTransferTimeout(15000);

    m_reply = m_network.get(request);
    connect(m_reply, &QNetworkReply::finished, this, &UpdateDialog::onReleaseReplyFinished);
}

void UpdateDialog::on_checkButton_pressed()
{
    checkUpdates();
}

void UpdateDialog::on_openReleaseButton_pressed()
{
    if (!m_releaseUrl.isValid())
        return;

    if (!QDesktopServices::openUrl(m_releaseUrl)) {
        QMessageBox::warning(
            this,
            "Tonatiuh++ Updates",
            QString("Could not open the release page:\n%1").arg(m_releaseUrl.toString())
        );
    }
}

void UpdateDialog::onReleaseReplyFinished()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply || reply != m_reply)
        return;

    QByteArray response = reply->readAll();
    QNetworkReply::NetworkError networkError = reply->error();
    QString errorText = reply->errorString();
    QVariant status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    int statusCode = status.isValid() ? status.toInt() : 0;

    reply->deleteLater();
    m_reply = nullptr;
    setChecking(false);

    if (networkError != QNetworkReply::NoError) {
        QString httpStatus = statusCode > 0 ? QString::number(statusCode) : "unavailable";
        showFailure(QString("Update check failed.\nHTTP status: %1\nNetwork error: %2").arg(httpStatus, errorText));
        return;
    }

    UpdateReader reader;
    if (!reader.currentVersionError().isEmpty()) {
        showFailure(
            QString("Installed application version is malformed.\nVersion: %1\nError: %2")
                .arg(reader.currentVersionText(), reader.currentVersionError())
        );
        return;
    }

    if (!reader.readGitHubRelease(response)) {
        showFailure(QString("Update check failed.\n%1").arg(reader.errorMessage()));
        return;
    }

    if (!reader.isUpdateAvailable()) {
        showResult(
            QString("Tonatiuh++ is up to date.\nInstalled version: %1\nLatest release: %2")
                .arg(reader.currentVersionText(), reader.latestTagName())
        );
        QMessageBox::information(this, "Tonatiuh++ Updates", "Tonatiuh++ is up to date.");
        return;
    }

    m_releaseUrl = reader.releaseUrl();
    setChecking(false);
    showResult(
        QString("Update available.\nInstalled version: %1\nLatest release: %2\n\nOpen the GitHub release page to download it.")
            .arg(reader.currentVersionText(), reader.latestTagName())
    );
    QMessageBox::information(this, "Tonatiuh++ Updates", "A newer Tonatiuh++ release is available.");
}

void UpdateDialog::setChecking(bool checking)
{
    ui->checkButton->setEnabled(!checking);
    ui->openReleaseButton->setEnabled(!checking && m_releaseUrl.isValid());
}

void UpdateDialog::showResult(const QString& message)
{
    ui->resultText->setPlainText(message);
}

void UpdateDialog::showFailure(const QString& message)
{
    m_releaseUrl = QUrl();
    setChecking(false);
    showResult(message);
    QMessageBox::warning(this, "Tonatiuh++ Updates", message);
}
