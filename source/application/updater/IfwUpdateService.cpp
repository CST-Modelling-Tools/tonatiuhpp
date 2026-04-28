#include "IfwUpdateService.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QStringList>

namespace
{
QStringList maintenanceToolNames()
{
#if defined(Q_OS_WIN)
    return { "MaintenanceTool.exe", "maintenancetool.exe" };
#elif defined(Q_OS_MACOS)
    return {
        "MaintenanceTool.app/Contents/MacOS/MaintenanceTool",
        "maintenancetool.app/Contents/MacOS/maintenancetool",
        "MaintenanceTool",
        "maintenancetool"
    };
#else
    return { "MaintenanceTool", "maintenancetool", "MaintenanceTool.run", "maintenancetool.run" };
#endif
}

bool isRunnableFile(const QString& path)
{
    QFileInfo info(path);
    return info.exists() && info.isFile() && info.isExecutable();
}

QString normalizedProcessText(const QByteArray& output, const QByteArray& errorOutput)
{
    QString text = QString::fromLocal8Bit(output);
    QString errorText = QString::fromLocal8Bit(errorOutput);
    if (!errorText.trimmed().isEmpty()) {
        if (!text.trimmed().isEmpty())
            text += "\n";
        text += errorText;
    }
    return text.trimmed();
}
}

IfwUpdateService::IfwUpdateService(QObject* parent):
    QObject(parent),
    m_process(nullptr),
    m_status(Idle),
    m_maintenanceToolPath(locateMaintenanceTool()),
    m_statusMessage("Update status has not been checked yet.")
{
}

IfwUpdateService::~IfwUpdateService()
{
    if (!m_process)
        return;

    m_process->disconnect(this);
    if (m_process->state() != QProcess::NotRunning)
        m_process->kill();
}

void IfwUpdateService::checkForUpdates()
{
    if (m_process && m_process->state() != QProcess::NotRunning)
        return;

    if (m_maintenanceToolPath.isEmpty())
        m_maintenanceToolPath = locateMaintenanceTool();

    if (m_maintenanceToolPath.isEmpty()) {
        setStatus(
            MaintenanceToolMissing,
            "Update checks require the Qt Installer Framework MaintenanceTool.",
            "MaintenanceTool was not found next to this Tonatiuh++ installation."
        );
        return;
    }

    if (!m_process) {
        m_process = new QProcess(this);
        connect(
            m_process,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this,
            [this](int exitCode, QProcess::ExitStatus exitStatus) {
                if (exitStatus != QProcess::NormalExit) {
                    failCheck("Update check failed.", "MaintenanceTool stopped unexpectedly.");
                    return;
                }
                finishCheck(exitCode);
            }
        );
        connect(
            m_process,
            &QProcess::errorOccurred,
            this,
            [this](QProcess::ProcessError) {
                failCheck("Update check failed.", m_process ? m_process->errorString() : QString());
            }
        );
    }

    setStatus(Checking, "Checking for updates...");
    QFileInfo toolInfo(m_maintenanceToolPath);
    m_process->setProgram(toolInfo.absoluteFilePath());
    m_process->setArguments({ "check-updates" });
    m_process->setWorkingDirectory(toolInfo.absolutePath());
    m_process->start();
}

bool IfwUpdateService::startUpdater(QString* errorMessage)
{
    if (errorMessage)
        errorMessage->clear();

    if (m_process && m_process->state() != QProcess::NotRunning) {
        if (errorMessage)
            *errorMessage = "The MaintenanceTool is still checking for updates.";
        return false;
    }

    if (m_maintenanceToolPath.isEmpty())
        m_maintenanceToolPath = locateMaintenanceTool();

    if (m_maintenanceToolPath.isEmpty()) {
        if (errorMessage)
            *errorMessage = "MaintenanceTool was not found next to this Tonatiuh++ installation.";
        return false;
    }

    QFileInfo toolInfo(m_maintenanceToolPath);
    qint64 processId = 0;
    bool started = QProcess::startDetached(
        toolInfo.absoluteFilePath(),
        { "--start-updater" },
        toolInfo.absolutePath(),
        &processId
    );
    if (!started && errorMessage)
        *errorMessage = QString("Could not start MaintenanceTool:\n%1").arg(toolInfo.absoluteFilePath());

    return started;
}

void IfwUpdateService::setStatus(Status status, const QString& message, const QString& details)
{
    m_status = status;
    m_statusMessage = message;
    m_updateDetails = details.trimmed();
    emit statusChanged(m_status);
}

void IfwUpdateService::finishCheck(int exitCode)
{
    QByteArray standardOutput = m_process->readAllStandardOutput();
    QByteArray standardError = m_process->readAllStandardError();
    QString output = normalizedProcessText(standardOutput, standardError);
    QString updateOutput = QString::fromLocal8Bit(standardOutput).trimmed();

    if (outputIndicatesNoUpdates(output)) {
        setStatus(UpToDate, "Tonatiuh++ is up to date.", output);
        return;
    }

    if (outputIndicatesUpdates(output) || !updateOutput.isEmpty()) {
        setStatus(UpdateAvailable, "Tonatiuh++ updates are available.", output);
        return;
    }

    if (exitCode == 0 || output.isEmpty()) {
        setStatus(UpToDate, "Tonatiuh++ is up to date.", output);
        return;
    }

    failCheck("Update check failed.", output);
}

void IfwUpdateService::failCheck(const QString& message, const QString& details)
{
    if (m_process && m_process->state() != QProcess::NotRunning)
        m_process->kill();
    setStatus(CheckFailed, message, details);
}

QString IfwUpdateService::locateMaintenanceTool()
{
    QStringList baseDirectories;
    QDir dir(QCoreApplication::applicationDirPath());
    for (int depth = 0; depth < 6; ++depth) {
        QString path = dir.absolutePath();
        if (!baseDirectories.contains(path))
            baseDirectories << path;
        if (!dir.cdUp())
            break;
    }

    const QStringList names = maintenanceToolNames();
    for (const QString& baseDirectory : baseDirectories) {
        QDir base(baseDirectory);
        for (const QString& name : names) {
            QString candidate = base.absoluteFilePath(name);
            if (isRunnableFile(candidate))
                return QFileInfo(candidate).absoluteFilePath();
        }
    }

    return QString();
}

bool IfwUpdateService::outputIndicatesUpdates(const QString& output)
{
    QString text = output.toLower();
    return text.contains("updates available") ||
        text.contains("update available") ||
        text.contains("available updates") ||
        text.contains("available update");
}

bool IfwUpdateService::outputIndicatesNoUpdates(const QString& output)
{
    QString text = output.toLower();
    return text.contains("no updates") ||
        text.contains("no update available") ||
        text.contains("no updates available") ||
        text.contains("there are currently no updates available");
}
