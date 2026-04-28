#pragma once

#include <QObject>
#include <QString>

class QProcess;

class IfwUpdateService : public QObject
{
    Q_OBJECT

public:
    enum Status {
        Idle,
        Checking,
        UpdateAvailable,
        UpToDate,
        MaintenanceToolMissing,
        CheckFailed
    };
    Q_ENUM(Status)

    explicit IfwUpdateService(QObject* parent = nullptr);
    ~IfwUpdateService();

    void checkForUpdates();

    Status status() const { return m_status; }
    bool isChecking() const { return m_status == Checking; }
    bool updateAvailable() const { return m_status == UpdateAvailable; }
    QString maintenanceToolPath() const { return m_maintenanceToolPath; }
    QString statusMessage() const { return m_statusMessage; }
    QString updateDetails() const { return m_updateDetails; }

signals:
    void statusChanged(IfwUpdateService::Status status);

private:
    void setStatus(Status status, const QString& message, const QString& details = QString());
    void finishCheck(int exitCode);
    void failCheck(const QString& message, const QString& details = QString());

    static QString locateMaintenanceTool();
    static bool outputIndicatesUpdates(const QString& output);
    static bool outputIndicatesNoUpdates(const QString& output);

    QProcess* m_process;
    Status m_status;
    QString m_maintenanceToolPath;
    QString m_statusMessage;
    QString m_updateDetails;
};
