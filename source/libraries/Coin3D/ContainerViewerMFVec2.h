#pragma once

#include "libraries/TonatiuhGuiPresentation.h"

#include <QDialog>

namespace Ui {
class ContainerViewerMFVec2;
}

class DoubleValuesDelegate;


class TONATIUH_GUI_PRESENTATION ContainerViewerMFVec2: public QDialog
{
    Q_OBJECT

public:
    ContainerViewerMFVec2(QWidget* parent = 0);
    ~ContainerViewerMFVec2();

public:
    QString GetData() const;
    bool SetData(QString value);

    void SetTitles(QStringList titles);

public slots:
    void AddNewRow();
    void DeleteSelectedRow();
    void HelpMenu();
    void CloseViewer();

protected:
    bool OkToContinue();

private:
    Ui::ContainerViewerMFVec2* ui;
};
