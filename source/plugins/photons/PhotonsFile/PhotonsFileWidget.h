#pragma once

#include "kernel/photons/PhotonsWidget.h"


namespace Ui {
class PhotonsFileWidget;
}

class TONATIUH_GUI_PRESENTATION PhotonsFileWidget: public PhotonsWidget
{
    Q_OBJECT

public:
    PhotonsFileWidget(QWidget* parent = 0);
    ~PhotonsFileWidget();

    QStringList getParameterNames() const;
    QString getParameterValue(QString name) const;
    void setParameterValue(QString /*name*/, QString /*value*/) {}// add setters

private slots:
    void selectDirectory();

private:
    Ui::PhotonsFileWidget* ui;
};
