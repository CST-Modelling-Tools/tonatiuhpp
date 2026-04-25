#pragma once

#include <QByteArray>
#include <QDialog>
#include <QFile>
#include <QNetworkAccessManager>
#include <QUrl>

class QNetworkReply;

namespace Ui {
class UpdateDialog;
}

class UpdateDialog : public QDialog
{
    Q_OBJECT

public:
    explicit UpdateDialog(QWidget* parent = 0);
    ~UpdateDialog();

    void checkUpdates();
    QString installerPathToStart() const { return m_installerPathToStart; }

private slots:
    void on_checkButton_pressed();
    void on_downloadButton_pressed();
    void onLatestReleaseReplyFinished();
    void onInstallerReadyRead();
    void onInstallerProgress(qint64 bytesReceived, qint64 bytesTotal);
    void onInstallerReplyFinished();
    void onChecksumReplyFinished();

private:
    void setChecking(bool checking);
    void setDownloading(bool downloading);
    void showResult(const QString& message);
    void showFailure(const QString& message);
    void beginInstallerDownload();
    void startChecksumDownload();
    void startInstallerDownload();
    void offerInstallUpdate();
    void startInstaller();

    Ui::UpdateDialog* ui;
    QNetworkAccessManager m_network;
    QNetworkReply* m_releaseReply;
    QNetworkReply* m_checksumReply;
    QNetworkReply* m_installerReply;
    QUrl m_installerUrl;
    QUrl m_checksumUrl;
    QString m_installerAssetName;
    QString m_checksumAssetName;
    qint64 m_installerAssetSize;
    qint64 m_checksumAssetSize;
    QByteArray m_expectedSha256;
    QFile m_installerFile;
    QString m_installerPath;
    QString m_partialInstallerPath;
    QString m_installerFileError;
    QString m_verifiedInstallerPath;
    QString m_installerPathToStart;
};
