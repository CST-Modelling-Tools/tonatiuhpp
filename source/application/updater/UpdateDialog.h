#pragma once

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

private slots:
    void on_checkButton_pressed();
    void on_downloadButton_pressed();
    void onReleaseReplyFinished();
    void onDownloadReadyRead();
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void onDownloadReplyFinished();

private:
    void setChecking(bool checking);
    void setDownloading(bool downloading);
    void showResult(const QString& message);
    void showFailure(const QString& message);
    void startDownload();

    Ui::UpdateDialog* ui;
    QNetworkAccessManager m_network;
    QNetworkReply* m_reply;
    QNetworkReply* m_downloadReply;
    QUrl m_downloadUrl;
    QString m_downloadAssetName;
    qint64 m_downloadAssetSize;
    QFile m_downloadFile;
    QString m_downloadPath;
    QString m_partialDownloadPath;
    QString m_downloadFileError;
};
