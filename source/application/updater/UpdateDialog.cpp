#include "UpdateDialog.h"
#include "ui_UpdateDialog.h"

#include "UpdateReader.h"

#include <QApplication>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPushButton>
#include <QStandardPaths>
#include <QStringList>

namespace
{
const QUrl kLatestReleaseUrl("https://api.github.com/repos/CST-Modelling-Tools/tonatiuhpp/releases/latest");

QString formatBytes(qint64 bytes)
{
    if (bytes < 0)
        return "unknown size";

    const QStringList units = { "B", "KB", "MB", "GB" };
    double value = static_cast<double>(bytes);
    int unitIndex = 0;
    while (value >= 1024.0 && unitIndex < units.size() - 1) {
        value /= 1024.0;
        ++unitIndex;
    }

    if (unitIndex == 0)
        return QString("%1 %2").arg(bytes).arg(units.at(unitIndex));

    return QString("%1 %2").arg(value, 0, 'f', 1).arg(units.at(unitIndex));
}

QString uniqueDownloadPath(const QString& directoryPath, const QString& fileName)
{
    QDir directory(directoryPath);
    QString safeFileName = QFileInfo(fileName).fileName();
    if (safeFileName.isEmpty())
        safeFileName = "tonatiuhpp-update";

    QString candidate = directory.filePath(safeFileName);
    if (!QFileInfo::exists(candidate))
        return candidate;

    QFileInfo info(safeFileName);
    QString suffix = info.completeSuffix();
    QString baseName = safeFileName;
    if (!suffix.isEmpty() && baseName.endsWith(QString(".%1").arg(suffix)))
        baseName.chop(suffix.size() + 1);

    for (int index = 1; index < 1000; ++index) {
        QString numberedName = suffix.isEmpty() ?
            QString("%1-%2").arg(baseName).arg(index) :
            QString("%1-%2.%3").arg(baseName).arg(index).arg(suffix);
        candidate = directory.filePath(numberedName);
        if (!QFileInfo::exists(candidate))
            return candidate;
    }

    return directory.filePath(QString("%1-%2").arg(safeFileName).arg(QDateTime::currentSecsSinceEpoch()));
}
}

UpdateDialog::UpdateDialog(QWidget* parent):
    QDialog(parent),
    ui(new Ui::UpdateDialog),
    m_reply(nullptr),
    m_downloadReply(nullptr),
    m_downloadAssetSize(-1)
{
    ui->setupUi(this);

    setWindowFlag(Qt::WindowContextHelpButtonHint, false);

    ui->downloadButton->setEnabled(false);
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

    if (m_downloadReply) {
        disconnect(m_downloadReply, nullptr, this, nullptr);
        m_downloadReply->abort();
        m_downloadReply->deleteLater();
    }

    if (m_downloadFile.isOpen())
        m_downloadFile.close();
    if (!m_partialDownloadPath.isEmpty())
        QFile::remove(m_partialDownloadPath);

    delete ui;
}

void UpdateDialog::checkUpdates()
{
    if (m_reply || m_downloadReply)
        return;

    m_downloadUrl = QUrl();
    m_downloadAssetName.clear();
    m_downloadAssetSize = -1;
    m_downloadPath.clear();
    m_partialDownloadPath.clear();
    m_downloadFileError.clear();
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

    if (!reader.hasDownloadAsset()) {
        showResult(
            QString("Update available.\nInstalled version: %1\nLatest release: %2\n\nNo update package was found for this platform.")
                .arg(reader.currentVersionText(), reader.latestTagName())
        );
        QMessageBox::warning(
            this,
            "Tonatiuh++ Updates",
            QString("A newer Tonatiuh++ release is available, but no update package was found for this platform.\n\nLatest release: %1")
                .arg(reader.latestTagName())
        );
        return;
    }

    m_downloadUrl = reader.downloadAssetUrl();
    m_downloadAssetName = reader.downloadAssetName();
    m_downloadAssetSize = reader.downloadAssetSize();
    setChecking(false);
    showResult(
        QString("Update available.\nInstalled version: %1\nLatest release: %2\nPackage: %3\nSize: %4")
            .arg(reader.currentVersionText(), reader.latestTagName(), m_downloadAssetName, formatBytes(m_downloadAssetSize))
    );

    QMessageBox updateMessage(this);
    updateMessage.setWindowTitle("Tonatiuh++ Updates");
    updateMessage.setIcon(QMessageBox::Information);
    updateMessage.setText(QString("Tonatiuh++ %1 is available.").arg(reader.latestTagName()));
    updateMessage.setInformativeText(
        QString("Package: %1\nSize: %2\n\nDo you want to download this update now?")
            .arg(m_downloadAssetName, formatBytes(m_downloadAssetSize))
    );
    QPushButton* downloadButton = updateMessage.addButton("Download", QMessageBox::AcceptRole);
    updateMessage.addButton("Later", QMessageBox::RejectRole);
    updateMessage.setDefaultButton(downloadButton);
    updateMessage.exec();

    if (updateMessage.clickedButton() == downloadButton)
        startDownload();
}

void UpdateDialog::setChecking(bool checking)
{
    ui->checkButton->setEnabled(!checking && !m_downloadReply);
    ui->downloadButton->setEnabled(!checking && !m_downloadReply && m_downloadUrl.isValid());
}

void UpdateDialog::setDownloading(bool downloading)
{
    ui->checkButton->setEnabled(!downloading && !m_reply);
    ui->downloadButton->setEnabled(!downloading && !m_reply && m_downloadUrl.isValid());
}

void UpdateDialog::showResult(const QString& message)
{
    ui->resultText->setPlainText(message);
}

void UpdateDialog::showFailure(const QString& message)
{
    setChecking(false);
    showResult(message);
    QMessageBox::warning(this, "Tonatiuh++ Updates", message);
}

void UpdateDialog::on_downloadButton_pressed()
{
    startDownload();
}

void UpdateDialog::startDownload()
{
    if (!m_downloadUrl.isValid() || m_downloadReply)
        return;

    QString downloadDirectory = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    if (downloadDirectory.isEmpty())
        downloadDirectory = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    if (downloadDirectory.isEmpty()) {
        showFailure("Update download failed.\nNo writable download directory is available.");
        return;
    }

    QDir directory(downloadDirectory);
    if (!directory.exists() && !directory.mkpath(".")) {
        showFailure(QString("Update download failed.\nCould not create download directory:\n%1").arg(downloadDirectory));
        return;
    }

    m_downloadPath = uniqueDownloadPath(downloadDirectory, m_downloadAssetName);
    m_partialDownloadPath = QString("%1.part").arg(m_downloadPath);
    m_downloadFileError.clear();

    QFile::remove(m_partialDownloadPath);
    m_downloadFile.setFileName(m_partialDownloadPath);
    if (!m_downloadFile.open(QIODevice::WriteOnly)) {
        showFailure(QString("Update download failed.\nCould not write to:\n%1\n%2").arg(m_partialDownloadPath, m_downloadFile.errorString()));
        return;
    }

    QNetworkRequest request(m_downloadUrl);
    request.setRawHeader("User-Agent", "TonatiuhPP");
    request.setTransferTimeout(60000);

    m_downloadReply = m_network.get(request);
    connect(m_downloadReply, &QNetworkReply::readyRead, this, &UpdateDialog::onDownloadReadyRead);
    connect(m_downloadReply, &QNetworkReply::downloadProgress, this, &UpdateDialog::onDownloadProgress);
    connect(m_downloadReply, &QNetworkReply::finished, this, &UpdateDialog::onDownloadReplyFinished);

    setDownloading(true);
    showResult(QString("Downloading update package...\nPackage: %1\nDestination: %2").arg(m_downloadAssetName, m_downloadPath));
}

void UpdateDialog::onDownloadReadyRead()
{
    if (!m_downloadReply || !m_downloadFile.isOpen())
        return;

    QByteArray data = m_downloadReply->readAll();
    if (data.isEmpty())
        return;

    if (m_downloadFile.write(data) != data.size()) {
        m_downloadFileError = m_downloadFile.errorString();
        m_downloadReply->abort();
    }
}

void UpdateDialog::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    qint64 total = bytesTotal > 0 ? bytesTotal : m_downloadAssetSize;
    showResult(
        QString("Downloading update package...\nPackage: %1\nProgress: %2 of %3\nDestination: %4")
            .arg(m_downloadAssetName, formatBytes(bytesReceived), formatBytes(total), m_downloadPath)
    );
}

void UpdateDialog::onDownloadReplyFinished()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply || reply != m_downloadReply)
        return;

    onDownloadReadyRead();

    QNetworkReply::NetworkError networkError = reply->error();
    QString errorText = reply->errorString();
    QVariant status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    int statusCode = status.isValid() ? status.toInt() : 0;

    reply->deleteLater();
    m_downloadReply = nullptr;

    if (m_downloadFile.isOpen()) {
        m_downloadFile.flush();
        m_downloadFile.close();
    }

    setDownloading(false);

    if (!m_downloadFileError.isEmpty()) {
        QFile::remove(m_partialDownloadPath);
        showFailure(QString("Update download failed.\nCould not write the update package:\n%1").arg(m_downloadFileError));
        return;
    }

    if (networkError != QNetworkReply::NoError) {
        QFile::remove(m_partialDownloadPath);
        QString httpStatus = statusCode > 0 ? QString::number(statusCode) : "unavailable";
        showFailure(QString("Update download failed.\nHTTP status: %1\nNetwork error: %2").arg(httpStatus, errorText));
        return;
    }

    if (QFileInfo::exists(m_downloadPath))
        QFile::remove(m_downloadPath);
    if (!QFile::rename(m_partialDownloadPath, m_downloadPath)) {
        QFile::remove(m_partialDownloadPath);
        showFailure(QString("Update download failed.\nCould not finalize the downloaded file:\n%1").arg(m_downloadPath));
        return;
    }

    m_downloadUrl = QUrl();
    setDownloading(false);

    showResult(QString("Update package downloaded.\nPackage: %1\nFile: %2").arg(m_downloadAssetName, m_downloadPath));
    QMessageBox::information(
        this,
        "Tonatiuh++ Updates",
        QString("The update package has been downloaded.\n\nFile: %1").arg(m_downloadPath)
    );
}
