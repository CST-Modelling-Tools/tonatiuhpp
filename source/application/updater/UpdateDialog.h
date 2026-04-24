#pragma once

#include <QDialog>
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
    void on_openReleaseButton_pressed();
    void onReleaseReplyFinished();

private:
    void setChecking(bool checking);
    void showResult(const QString& message);
    void showFailure(const QString& message);

    Ui::UpdateDialog* ui;
    QNetworkAccessManager m_network;
    QNetworkReply* m_reply;
    QUrl m_releaseUrl;
};
