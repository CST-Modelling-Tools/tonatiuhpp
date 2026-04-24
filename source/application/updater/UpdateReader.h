#pragma once

#include <QByteArray>
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
    QString errorMessage() const { return m_message; }

    static QString normalizedVersionTag(const QString& tag);
    static bool parseDottedVersion(const QString& versionText, QVersionNumber* version, QString* error = nullptr);

private:
    QString m_message;

    QString m_currentVersionText;
    QString m_currentVersionError;
    QVersionNumber m_currentVersion;

    QString m_latestTagName;
    QString m_latestVersionText;
    QVersionNumber m_latestVersion;
    QUrl m_releaseUrl;
};
