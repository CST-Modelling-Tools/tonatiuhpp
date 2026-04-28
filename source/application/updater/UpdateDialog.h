#pragma once

#include <QDialog>

class IfwUpdateService;

namespace Ui {
class UpdateDialog;
}

class UpdateDialog : public QDialog
{
    Q_OBJECT

public:
    explicit UpdateDialog(IfwUpdateService* updateService, QWidget* parent = nullptr);
    ~UpdateDialog();

    void checkUpdates();
    bool updateRequested() const { return m_updateRequested; }

private slots:
    void on_checkButton_pressed();
    void on_downloadButton_pressed();

private:
    void refresh();
    void showMessage(const QString& message);

    Ui::UpdateDialog* ui;
    IfwUpdateService* m_updateService;
    bool m_updateRequested;
};
