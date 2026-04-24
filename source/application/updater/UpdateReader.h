#pragma once

#include <QByteArray>
#include <QDate>
#include <QList>
#include <QString>
#include <QUrl>
#include <QVersionNumber>

class UpdateReader
{
public:
    UpdateReader();

    bool readGitHubRelease(const QByteArray& data);
    bool isUpdateAvailable() const;

    QString currentVersionText() const { return m_currentVersionText; }
    QString currentVersionError() const { return m_currentVersionError; }
    QString latestTagName() const { return m_latestTagName; }
    QString latestVersionText() const { return m_latestVersionText; }
    QUrl releaseUrl() const { return m_releaseUrl; }

    static QString normalizedVersionTag(const QString& tag);
    static bool parseDottedVersion(const QString& versionText, QVersionNumber* version, QString* error = nullptr);

    bool checkUpdates(const QString& data);
    bool isNewer();

    QList<int> m_versionCurrent;

    QDate m_date;
    QList<int> m_version;
    int m_size;
    QString m_path;

    QString m_message;

protected:
    QList<int> toVersion(const QString& version) const;
    bool isGrowing(const QList<int>& a, const QList<int>& b) const;

private:
    QString m_currentVersionText;
    QString m_currentVersionError;
    QVersionNumber m_currentVersion;

    QString m_latestTagName;
    QString m_latestVersionText;
    QVersionNumber m_latestVersion;
    QUrl m_releaseUrl;
};
