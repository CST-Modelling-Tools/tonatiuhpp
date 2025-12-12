#include "AboutDialog.h"
#include "ui_AboutDialog.h"

#include <QSettings>
#include <QDate>
#include <QIcon>
#include <QDir>

AboutDialog::AboutDialog(QWidget* parent):
    QDialog(parent),
    ui(new Ui::AboutDialog)
{
    ui->setupUi(this);

    QString textVersion = qApp->applicationVersion();

    QString textDate = QString(__DATE__).simplified(); // "Jan  1 2020"
    QDate date = QDate::fromString(textDate, "MMM d yyyy");
    textDate = date.toString("d MMM yyyy");

    QString text = QString("Version: %1 (%2)").arg(textVersion, textDate);

    ui->label_version->setText(text);

    setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    setWindowFlag(Qt::MSWindowsFixedSizeDialogHint, true);

    updateLabel();
    qDebug() << QDir::searchPaths("project");
}

AboutDialog::~AboutDialog()
{
    delete ui;
}

void AboutDialog::updateLabel()
{
    QSettings settings("Tonatiuh", "Cyprus");
    QString theme = settings.value("theme", "").toString();

    QString fileIcon;
    QString filePixmap;
    if (theme == "") {
        fileIcon = ":/images/about/Tonatiuh.ico";
        filePixmap = ":/images/about/SplashScreen.png";
    } else {
        fileIcon = ":/images/about/Tonatiuh.ico";
        filePixmap = ":/images/about/SplashScreen.png";
    }

    qApp->setWindowIcon(QIcon(fileIcon));

    QPixmap pixmap(filePixmap);
    pixmap.setDevicePixelRatio(qApp->devicePixelRatio());
    int q = fontMetrics().height();
    ui->label->setPixmap(pixmap.scaledToHeight(22*q, Qt::SmoothTransformation));
}

void AboutDialog::on_label_customContextMenuRequested(const QPoint& pos)
{
    Q_UNUSED(pos)

    QSettings settings("Tonatiuh", "Cyprus");
    QString theme = settings.value("theme", "").toString();

    if (theme == "")
        theme = "Cyprus";
    else
        theme = "";

    settings.setValue("theme", theme);

    updateLabel();
}
