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
    QString installerAssetName() const { return m_installerAssetName; }
    QUrl installerAssetUrl() const { return m_installerAssetUrl; }
    qint64 installerAssetSize() const { return m_installerAssetSize; }
    bool hasInstallerAsset() const { return m_installerAssetUrl.isValid(); }
    QString checksumAssetName() const { return m_checksumAssetName; }
    QUrl checksumAssetUrl() const { return m_checksumAssetUrl; }
    qint64 checksumAssetSize() const { return m_checksumAssetSize; }
    bool hasChecksumAsset() const { return m_checksumAssetUrl.isValid(); }
    QString errorMessage() const { return m_message; }

    static QString normalizedVersionTag(const QString& tag);
    static bool parseDottedVersion(const QString& versionText, QVersionNumber* version, QString* error = nullptr);
    static bool parseSha256Checksum(const QByteArray& data, const QString& expectedFileName, QByteArray* checksum, QString* error = nullptr);
    static bool isCurrentPlatformInstallerAsset(const QString& assetName, const QString& versionText);

private:
    QString m_message;

    QString m_currentVersionText;
    QString m_currentVersionError;
    QVersionNumber m_currentVersion;

    QString m_latestTagName;
    QString m_latestVersionText;
    QVersionNumber m_latestVersion;
    QUrl m_releaseUrl;
    QString m_installerAssetName;
    QUrl m_installerAssetUrl;
    qint64 m_installerAssetSize;
    QString m_checksumAssetName;
    QUrl m_checksumAssetUrl;
    qint64 m_checksumAssetSize;
};
