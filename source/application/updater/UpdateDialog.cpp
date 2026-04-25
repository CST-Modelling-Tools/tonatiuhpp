#include "UpdateDialog.h"
#include "ui_UpdateDialog.h"

#include "UpdateReader.h"

#include <QApplication>
#include <QCryptographicHash>
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
const QUrl kLatestReleaseApiUrl("https://api.github.com/repos/CST-Modelling-Tools/tonatiuhpp/releases/latest");

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

QByteArray fileSha256(const QString& path, QString* error)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        if (error)
            *error = file.errorString();
        return QByteArray();
    }

    QCryptographicHash hash(QCryptographicHash::Sha256);
    while (!file.atEnd()) {
        QByteArray chunk = file.read(1024 * 1024);
        if (chunk.isEmpty() && file.error() != QFileDevice::NoError) {
            if (error)
                *error = file.errorString();
            return QByteArray();
        }
        hash.addData(chunk);
    }

    if (error)
        error->clear();
    return hash.result().toHex();
}

bool isRunnableInstaller(const QString& path)
{
#if defined(Q_OS_WIN)
    return path.endsWith(".exe", Qt::CaseInsensitive);
#else
    Q_UNUSED(path);
    return false;
#endif
}

void allowHttpsRedirects(QNetworkRequest& request)
{
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
}

QString formatHttpStatus(int statusCode)
{
    return statusCode > 0 ? QString::number(statusCode) : "unavailable";
}

bool isSuccessfulGetStatus(int statusCode)
{
    return statusCode == 200;
}

bool isValidFinalReplyUrl(const QUrl& url)
{
    return url.isValid() && url.scheme() == "https";
}

bool isExpectedLatestReleaseApiReplyUrl(const QUrl& url)
{
    return url.isValid() &&
        url.scheme() == kLatestReleaseApiUrl.scheme() &&
        url.host().compare(kLatestReleaseApiUrl.host(), Qt::CaseInsensitive) == 0 &&
        url.path() == kLatestReleaseApiUrl.path() &&
        url.query().isEmpty() &&
        url.fragment().isEmpty();
}
}

UpdateDialog::UpdateDialog(QWidget* parent):
    QDialog(parent),
    ui(new Ui::UpdateDialog),
    m_releaseReply(nullptr),
    m_checksumReply(nullptr),
    m_installerReply(nullptr),
    m_installerAssetSize(-1),
    m_checksumAssetSize(-1)
{
    ui->setupUi(this);

    setWindowFlag(Qt::WindowContextHelpButtonHint, false);

    ui->downloadButton->setEnabled(false);
    showResult(QString("Installed version: %1").arg(qApp->applicationVersion()));

    ui->checkButton->setFocus();
}

UpdateDialog::~UpdateDialog()
{
    if (m_releaseReply) {
        disconnect(m_releaseReply, nullptr, this, nullptr);
        m_releaseReply->abort();
        m_releaseReply->deleteLater();
    }

    if (m_installerReply) {
        disconnect(m_installerReply, nullptr, this, nullptr);
        m_installerReply->abort();
        m_installerReply->deleteLater();
    }

    if (m_checksumReply) {
        disconnect(m_checksumReply, nullptr, this, nullptr);
        m_checksumReply->abort();
        m_checksumReply->deleteLater();
    }

    if (m_installerFile.isOpen())
        m_installerFile.close();
    if (!m_partialInstallerPath.isEmpty())
        QFile::remove(m_partialInstallerPath);

    delete ui;
}

void UpdateDialog::checkUpdates()
{
    if (m_releaseReply || m_checksumReply || m_installerReply)
        return;

    m_installerUrl = QUrl();
    m_checksumUrl = QUrl();
    m_installerAssetName.clear();
    m_checksumAssetName.clear();
    m_installerAssetSize = -1;
    m_checksumAssetSize = -1;
    m_expectedSha256.clear();
    m_installerPath.clear();
    m_partialInstallerPath.clear();
    m_installerFileError.clear();
    m_verifiedInstallerPath.clear();
    m_installerPathToStart.clear();
    ui->downloadButton->setText("Download Installer");
    setChecking(true);
    showResult(QString("Checking for updates...\nInstalled version: %1").arg(qApp->applicationVersion()));

    QNetworkRequest request(kLatestReleaseApiUrl);
    request.setRawHeader("Accept", "application/vnd.github+json");
    request.setRawHeader("User-Agent", "TonatiuhPP");
    request.setTransferTimeout(15000);
    allowHttpsRedirects(request);

    m_releaseReply = m_network.get(request);
    connect(m_releaseReply, &QNetworkReply::finished, this, &UpdateDialog::onLatestReleaseReplyFinished);
}

void UpdateDialog::on_checkButton_pressed()
{
    checkUpdates();
}

void UpdateDialog::onLatestReleaseReplyFinished()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply || reply != m_releaseReply)
        return;

    QByteArray response = reply->readAll();
    QNetworkReply::NetworkError networkError = reply->error();
    QString errorText = reply->errorString();
    QVariant status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    int statusCode = status.isValid() ? status.toInt() : 0;
    QString httpStatus = formatHttpStatus(statusCode);
    QUrl finalUrl = reply->url();

    reply->deleteLater();
    m_releaseReply = nullptr;
    setChecking(false);

    if (networkError != QNetworkReply::NoError) {
        showFailure(QString("Update check failed.\nHTTP status: %1\nNetwork error: %2").arg(httpStatus, errorText));
        return;
    }

    if (!isSuccessfulGetStatus(statusCode)) {
        showFailure(QString("Update check failed.\nUnexpected HTTP status: %1").arg(httpStatus));
        return;
    }

    if (!isExpectedLatestReleaseApiReplyUrl(finalUrl)) {
        showFailure(QString("Update check failed.\nUnexpected final URL: %1").arg(finalUrl.toString()));
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

    if (!reader.hasInstallerAsset()) {
        showResult(
            QString("Update available.\nInstalled version: %1\nLatest release: %2\n\nNo self-update installer was found for this platform.")
                .arg(reader.currentVersionText(), reader.latestTagName())
        );
        QMessageBox::warning(
            this,
            "Tonatiuh++ Updates",
            QString("A newer Tonatiuh++ release is available, but this release does not include a self-update installer for this platform.\n\nLatest release: %1")
                .arg(reader.latestTagName())
        );
        return;
    }

    if (!reader.hasChecksumAsset()) {
        showResult(
            QString("Update available.\nInstalled version: %1\nLatest release: %2\nInstaller: %3\n\nNo checksum file was found for this installer, so it will not be downloaded.")
                .arg(reader.currentVersionText(), reader.latestTagName(), reader.installerAssetName())
        );
        QMessageBox::warning(
            this,
            "Tonatiuh++ Updates",
            QString("A newer Tonatiuh++ release is available, but its installer is missing a checksum file.\n\nInstaller: %1")
                .arg(reader.installerAssetName())
        );
        return;
    }

    m_installerUrl = reader.installerAssetUrl();
    m_checksumUrl = reader.checksumAssetUrl();
    m_installerAssetName = reader.installerAssetName();
    m_checksumAssetName = reader.checksumAssetName();
    m_installerAssetSize = reader.installerAssetSize();
    m_checksumAssetSize = reader.checksumAssetSize();
    setChecking(false);
    showResult(
        QString("Update available.\nInstalled version: %1\nLatest release: %2\nInstaller: %3\nSize: %4")
            .arg(reader.currentVersionText(), reader.latestTagName(), m_installerAssetName, formatBytes(m_installerAssetSize))
    );

    QMessageBox updateMessage(this);
    updateMessage.setWindowTitle("Tonatiuh++ Updates");
    updateMessage.setIcon(QMessageBox::Information);
    updateMessage.setText(QString("Tonatiuh++ %1 is available.").arg(reader.latestTagName()));
    updateMessage.setInformativeText(
        QString("Installer: %1\nSize: %2\n\nDo you want to download and verify this installer now?")
            .arg(m_installerAssetName, formatBytes(m_installerAssetSize))
    );
    QPushButton* downloadButton = updateMessage.addButton("Download", QMessageBox::AcceptRole);
    updateMessage.addButton("Later", QMessageBox::RejectRole);
    updateMessage.setDefaultButton(downloadButton);
    updateMessage.exec();

    if (updateMessage.clickedButton() == downloadButton)
        startUpdateDownload();
}

void UpdateDialog::setChecking(bool checking)
{
    ui->checkButton->setEnabled(!checking && !m_checksumReply && !m_installerReply);
    ui->downloadButton->setEnabled(
        !checking &&
        !m_checksumReply &&
        !m_installerReply &&
        (!m_verifiedInstallerPath.isEmpty() || (m_installerUrl.isValid() && m_checksumUrl.isValid()))
    );
}

void UpdateDialog::setDownloading(bool downloading)
{
    ui->checkButton->setEnabled(!downloading && !m_releaseReply);
    ui->downloadButton->setEnabled(
        !downloading &&
        !m_releaseReply &&
        (!m_verifiedInstallerPath.isEmpty() || (m_installerUrl.isValid() && m_checksumUrl.isValid()))
    );
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
    if (!m_verifiedInstallerPath.isEmpty()) {
        startInstaller();
        return;
    }

    startUpdateDownload();
}

void UpdateDialog::startUpdateDownload()
{
    if (!m_installerUrl.isValid() || !m_checksumUrl.isValid() || m_checksumReply || m_installerReply)
        return;

    QString downloadDirectory = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    if (downloadDirectory.isEmpty())
        downloadDirectory = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    if (downloadDirectory.isEmpty()) {
        showFailure("Installer download failed.\nNo writable download directory is available.");
        return;
    }

    QDir directory(downloadDirectory);
    if (!directory.exists() && !directory.mkpath(".")) {
        showFailure(QString("Installer download failed.\nCould not create download directory:\n%1").arg(downloadDirectory));
        return;
    }

    m_installerPath = uniqueDownloadPath(downloadDirectory, m_installerAssetName);
    m_partialInstallerPath = QString("%1.part").arg(m_installerPath);
    m_installerFileError.clear();
    m_expectedSha256.clear();
    m_verifiedInstallerPath.clear();
    ui->downloadButton->setText("Download Installer");

    startChecksumDownload();
}

void UpdateDialog::startChecksumDownload()
{
    QNetworkRequest request(m_checksumUrl);
    request.setRawHeader("User-Agent", "TonatiuhPP");
    request.setTransferTimeout(30000);
    allowHttpsRedirects(request);

    m_checksumReply = m_network.get(request);
    connect(m_checksumReply, &QNetworkReply::finished, this, &UpdateDialog::onChecksumReplyFinished);

    setDownloading(true);
    showResult(QString("Downloading installer checksum...\nChecksum: %1\nInstaller: %2").arg(m_checksumAssetName, m_installerAssetName));
}

void UpdateDialog::onChecksumReplyFinished()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply || reply != m_checksumReply)
        return;

    QByteArray response = reply->readAll();
    QNetworkReply::NetworkError networkError = reply->error();
    QString errorText = reply->errorString();
    QVariant status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    int statusCode = status.isValid() ? status.toInt() : 0;
    QString httpStatus = formatHttpStatus(statusCode);
    QUrl finalUrl = reply->url();

    reply->deleteLater();
    m_checksumReply = nullptr;

    if (networkError != QNetworkReply::NoError) {
        setDownloading(false);
        showFailure(QString("Installer checksum download failed.\nHTTP status: %1\nNetwork error: %2").arg(httpStatus, errorText));
        return;
    }

    if (!isSuccessfulGetStatus(statusCode)) {
        setDownloading(false);
        showFailure(QString("Installer checksum download failed.\nUnexpected HTTP status: %1").arg(httpStatus));
        return;
    }

    if (!isValidFinalReplyUrl(finalUrl)) {
        setDownloading(false);
        showFailure(QString("Installer checksum download failed.\nUnexpected final URL: %1").arg(finalUrl.toString()));
        return;
    }

    if (m_checksumAssetSize > 0 && response.size() != m_checksumAssetSize) {
        setDownloading(false);
        showFailure(
            QString("Installer checksum download failed.\nThe downloaded checksum size does not match the release metadata.\nExpected: %1\nActual: %2")
                .arg(formatBytes(m_checksumAssetSize), formatBytes(response.size()))
        );
        return;
    }

    QString checksumError;
    if (!UpdateReader::parseSha256Checksum(response, m_installerAssetName, &m_expectedSha256, &checksumError)) {
        setDownloading(false);
        showFailure(QString("Update checksum is malformed.\n%1").arg(checksumError));
        return;
    }

    startInstallerDownload();
}

void UpdateDialog::startInstallerDownload()
{
    QFile::remove(m_partialInstallerPath);
    m_installerFile.setFileName(m_partialInstallerPath);
    if (!m_installerFile.open(QIODevice::WriteOnly)) {
        showFailure(QString("Installer download failed.\nCould not write to:\n%1\n%2").arg(m_partialInstallerPath, m_installerFile.errorString()));
        return;
    }

    QNetworkRequest request(m_installerUrl);
    request.setRawHeader("User-Agent", "TonatiuhPP");
    request.setTransferTimeout(60000);
    allowHttpsRedirects(request);

    m_installerReply = m_network.get(request);
    connect(m_installerReply, &QNetworkReply::readyRead, this, &UpdateDialog::onInstallerReadyRead);
    connect(m_installerReply, &QNetworkReply::downloadProgress, this, &UpdateDialog::onInstallerProgress);
    connect(m_installerReply, &QNetworkReply::finished, this, &UpdateDialog::onInstallerReplyFinished);

    showResult(QString("Downloading installer...\nInstaller: %1\nDestination: %2").arg(m_installerAssetName, m_installerPath));
}

void UpdateDialog::onInstallerReadyRead()
{
    if (!m_installerReply || !m_installerFile.isOpen())
        return;

    QByteArray data = m_installerReply->readAll();
    if (data.isEmpty())
        return;

    if (m_installerFile.write(data) != data.size()) {
        m_installerFileError = m_installerFile.errorString();
        m_installerReply->abort();
    }
}

void UpdateDialog::onInstallerProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    qint64 total = bytesTotal > 0 ? bytesTotal : m_installerAssetSize;
    showResult(
        QString("Downloading installer...\nInstaller: %1\nProgress: %2 of %3\nDestination: %4")
            .arg(m_installerAssetName, formatBytes(bytesReceived), formatBytes(total), m_installerPath)
    );
}

void UpdateDialog::onInstallerReplyFinished()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply || reply != m_installerReply)
        return;

    onInstallerReadyRead();

    QNetworkReply::NetworkError networkError = reply->error();
    QString errorText = reply->errorString();
    QVariant status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    int statusCode = status.isValid() ? status.toInt() : 0;
    QString httpStatus = formatHttpStatus(statusCode);
    QUrl finalUrl = reply->url();

    reply->deleteLater();
    m_installerReply = nullptr;

    if (m_installerFile.isOpen()) {
        m_installerFile.flush();
        m_installerFile.close();
    }

    setDownloading(false);

    if (!m_installerFileError.isEmpty()) {
        QFile::remove(m_partialInstallerPath);
        showFailure(QString("Installer download failed.\nCould not write the installer:\n%1").arg(m_installerFileError));
        return;
    }

    if (networkError != QNetworkReply::NoError) {
        QFile::remove(m_partialInstallerPath);
        showFailure(QString("Installer download failed.\nHTTP status: %1\nNetwork error: %2").arg(httpStatus, errorText));
        return;
    }

    if (!isSuccessfulGetStatus(statusCode)) {
        QFile::remove(m_partialInstallerPath);
        showFailure(QString("Installer download failed.\nUnexpected HTTP status: %1").arg(httpStatus));
        return;
    }

    if (!isValidFinalReplyUrl(finalUrl)) {
        QFile::remove(m_partialInstallerPath);
        showFailure(QString("Installer download failed.\nUnexpected final URL: %1").arg(finalUrl.toString()));
        return;
    }

    if (QFileInfo::exists(m_installerPath))
        QFile::remove(m_installerPath);
    if (!QFile::rename(m_partialInstallerPath, m_installerPath)) {
        QFile::remove(m_partialInstallerPath);
        showFailure(QString("Installer download failed.\nCould not finalize the downloaded file:\n%1").arg(m_installerPath));
        return;
    }

    qint64 actualFileSize = QFileInfo(m_installerPath).size();
    if (m_installerAssetSize > 0 && actualFileSize != m_installerAssetSize) {
        QFile::remove(m_installerPath);
        showFailure(
            QString("Update verification failed.\nThe downloaded installer size does not match the release metadata.\nExpected: %1\nActual: %2")
                .arg(formatBytes(m_installerAssetSize), formatBytes(actualFileSize))
        );
        return;
    }

    QString hashError;
    QByteArray actualSha256 = fileSha256(m_installerPath, &hashError);
    if (actualSha256.isEmpty()) {
        QFile::remove(m_installerPath);
        showFailure(QString("Update verification failed.\nCould not read the downloaded installer:\n%1").arg(hashError));
        return;
    }

    if (actualSha256 != m_expectedSha256) {
        QFile::remove(m_installerPath);
        showFailure(
            QString("Update verification failed.\nThe downloaded installer checksum does not match the release checksum.\nExpected: %1\nActual: %2")
                .arg(QString::fromLatin1(m_expectedSha256), QString::fromLatin1(actualSha256))
        );
        return;
    }

    if (isRunnableInstaller(m_installerPath)) {
        m_verifiedInstallerPath = m_installerPath;
        ui->downloadButton->setText("Start Installer and Close");
    }

    m_installerUrl = QUrl();
    m_checksumUrl = QUrl();
    setDownloading(false);
    showResult(QString("Update installer downloaded and verified.\nInstaller: %1\nFile: %2").arg(m_installerAssetName, m_installerPath));
    offerInstallUpdate();
}

void UpdateDialog::offerInstallUpdate()
{
    if (m_verifiedInstallerPath.isEmpty()) {
        QMessageBox::information(
            this,
            "Tonatiuh++ Updates",
            QString("The update installer has been downloaded and verified.\n\nFile: %1").arg(m_installerPath)
        );
        return;
    }

    QMessageBox installMessage(this);
    installMessage.setWindowTitle("Tonatiuh++ Updates");
    installMessage.setIcon(QMessageBox::Information);
    installMessage.setText("The update installer has been downloaded and verified.");
    installMessage.setInformativeText(
        QString("Installer: %1\n\nDo you want to start the installer and close Tonatiuh++ now?\nIf there are unsaved changes, Tonatiuh++ will ask what to do before closing.")
            .arg(m_verifiedInstallerPath)
    );
    QPushButton* installButton = installMessage.addButton("Start Installer and Close", QMessageBox::AcceptRole);
    installMessage.addButton("Later", QMessageBox::RejectRole);
    installMessage.setDefaultButton(installButton);
    installMessage.exec();

    if (installMessage.clickedButton() != installButton)
        return;

    startInstaller();
}

void UpdateDialog::startInstaller()
{
    if (m_verifiedInstallerPath.isEmpty())
        return;

    m_installerPathToStart = m_verifiedInstallerPath;
    accept();
}
