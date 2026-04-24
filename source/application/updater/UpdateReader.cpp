#include "UpdateReader.h"

#include <cmath>
#include <limits>

#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QRegularExpression>


namespace
{
int compareVersions(const QVersionNumber& a, const QVersionNumber& b)
{
    int count = a.segmentCount() > b.segmentCount() ? a.segmentCount() : b.segmentCount();
    for (int i = 0; i < count; ++i) {
        int av = i < a.segmentCount() ? a.segmentAt(i) : 0;
        int bv = i < b.segmentCount() ? b.segmentAt(i) : 0;
        if (av < bv)
            return -1;
        if (av > bv)
            return 1;
    }
    return 0;
}

bool isExpectedGitHubReleaseAssetUrl(const QUrl& url, const QString& tagName, const QString& assetName)
{
    QString expectedTag = tagName.trimmed();
    QString expectedAsset = assetName.trimmed();
    if (expectedTag.isEmpty() || expectedAsset.isEmpty())
        return false;

    QString expectedPath = QString("/CST-Modelling-Tools/tonatiuhpp/releases/download/%1/%2")
        .arg(expectedTag, expectedAsset);

    return url.isValid() &&
        url.scheme() == "https" &&
        url.host().compare("github.com", Qt::CaseInsensitive) == 0 &&
        url.path() == expectedPath &&
        url.query().isEmpty() &&
        url.fragment().isEmpty();
}

bool isExpectedGitHubReleasePageUrl(const QUrl& url, const QString& tagName)
{
    QString expectedTag = tagName.trimmed();
    if (expectedTag.isEmpty())
        return false;

    QString expectedPath = QString("/CST-Modelling-Tools/tonatiuhpp/releases/tag/%1").arg(expectedTag);

    return url.isValid() &&
        url.scheme() == "https" &&
        url.host().compare("github.com", Qt::CaseInsensitive) == 0 &&
        url.path() == expectedPath &&
        url.query().isEmpty() &&
        url.fragment().isEmpty();
}

bool isChecksumForInstallerAsset(const QString& checksumAssetName, const QString& installerAssetName)
{
    return checksumAssetName.compare(QString("%1.sha256").arg(installerAssetName), Qt::CaseInsensitive) == 0;
}

bool isValidGitHubAssetSize(const QJsonValue& sizeValue)
{
    double sizeDouble = sizeValue.toDouble(-1.0);
    return sizeValue.isDouble() &&
        sizeDouble > 0.0 &&
        sizeDouble <= static_cast<double>(std::numeric_limits<qint64>::max()) &&
        std::floor(sizeDouble) == sizeDouble;
}
}

UpdateReader::UpdateReader()
{
    m_currentVersionText = QCoreApplication::applicationVersion();
    parseDottedVersion(m_currentVersionText, &m_currentVersion, &m_currentVersionError);
    m_installerAssetSize = -1;
    m_checksumAssetSize = -1;
}

bool UpdateReader::readGitHubRelease(const QByteArray& data)
{
    m_message.clear();
    m_latestTagName.clear();
    m_latestVersionText.clear();
    m_latestVersion = QVersionNumber();
    m_releaseUrl = QUrl();
    m_installerAssetName.clear();
    m_installerAssetUrl = QUrl();
    m_installerAssetSize = -1;
    m_checksumAssetName.clear();
    m_checksumAssetUrl = QUrl();
    m_checksumAssetSize = -1;

    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        m_message = QString("GitHub release response is not valid JSON: %1").arg(parseError.errorString());
        return false;
    }

    if (!document.isObject()) {
        m_message = "GitHub release response is not a JSON object";
        return false;
    }

    QJsonObject release = document.object();
    QJsonValue tagValue = release.value("tag_name");
    if (!tagValue.isString() || tagValue.toString().trimmed().isEmpty()) {
        m_message = "GitHub release response is missing tag_name";
        return false;
    }

    m_latestTagName = tagValue.toString().trimmed();
    m_latestVersionText = normalizedVersionTag(m_latestTagName);

    QString versionError;
    if (!parseDottedVersion(m_latestVersionText, &m_latestVersion, &versionError)) {
        m_message = QString("Malformed GitHub release tag \"%1\": %2").arg(m_latestTagName, versionError);
        return false;
    }

    QJsonValue urlValue = release.value("html_url");
    if (!urlValue.isString() || urlValue.toString().trimmed().isEmpty()) {
        m_message = "GitHub release response is missing html_url";
        return false;
    }

    m_releaseUrl = QUrl(urlValue.toString().trimmed());
    if (!m_releaseUrl.isValid() || m_releaseUrl.scheme() != "https") {
        m_message = QString("GitHub release response contains an invalid or non-HTTPS html_url: %1").arg(urlValue.toString());
        return false;
    }
    if (!isExpectedGitHubReleasePageUrl(m_releaseUrl, m_latestTagName)) {
        m_message = QString("GitHub release response contains an unexpected html_url: %1").arg(urlValue.toString());
        return false;
    }

    QJsonValue assetsValue = release.value("assets");
    if (assetsValue.isUndefined() || assetsValue.isNull())
        return true;

    if (!assetsValue.isArray()) {
        m_message = "GitHub release response contains malformed assets";
        return false;
    }

    QJsonArray assets = assetsValue.toArray();
    for (const QJsonValue& assetValue : assets) {
        if (!assetValue.isObject()) {
            m_message = "GitHub release response contains a malformed asset";
            return false;
        }

        QJsonObject asset = assetValue.toObject();
        QJsonValue nameValue = asset.value("name");
        if (!nameValue.isString() || nameValue.toString().trimmed().isEmpty()) {
            m_message = "GitHub release response contains an asset without a name";
            return false;
        }

        QString assetName = nameValue.toString().trimmed();
        if (!m_installerAssetName.isEmpty() || !isCurrentPlatformInstallerAsset(assetName, m_latestVersionText))
            continue;

        QJsonValue downloadUrlValue = asset.value("browser_download_url");
        if (!downloadUrlValue.isString() || downloadUrlValue.toString().trimmed().isEmpty()) {
            m_message = QString("GitHub release asset \"%1\" is missing browser_download_url").arg(assetName);
            return false;
        }

        QUrl downloadUrl(downloadUrlValue.toString().trimmed());
        if (!isExpectedGitHubReleaseAssetUrl(downloadUrl, m_latestTagName, assetName)) {
            m_message = QString("GitHub release asset \"%1\" contains an unexpected browser_download_url: %2")
                .arg(assetName, downloadUrlValue.toString());
            return false;
        }

        m_installerAssetName = assetName;
        m_installerAssetUrl = downloadUrl;

        QJsonValue sizeValue = asset.value("size");
        if (!isValidGitHubAssetSize(sizeValue)) {
            m_message = QString("GitHub release asset \"%1\" contains an invalid size").arg(assetName);
            return false;
        }
        m_installerAssetSize = static_cast<qint64>(sizeValue.toDouble());

    }

    if (!m_installerAssetName.isEmpty()) {
        for (const QJsonValue& assetValue : assets) {
            QJsonObject asset = assetValue.toObject();
            QString assetName = asset.value("name").toString().trimmed();
            if (!isChecksumForInstallerAsset(assetName, m_installerAssetName))
                continue;

            QJsonValue downloadUrlValue = asset.value("browser_download_url");
            if (!downloadUrlValue.isString() || downloadUrlValue.toString().trimmed().isEmpty()) {
                m_message = QString("GitHub release checksum asset \"%1\" is missing browser_download_url").arg(assetName);
                return false;
            }

            QUrl downloadUrl(downloadUrlValue.toString().trimmed());
            if (!isExpectedGitHubReleaseAssetUrl(downloadUrl, m_latestTagName, assetName)) {
                m_message = QString("GitHub release checksum asset \"%1\" contains an unexpected browser_download_url: %2")
                    .arg(assetName, downloadUrlValue.toString());
                return false;
            }

            if (!isValidGitHubAssetSize(asset.value("size"))) {
                m_message = QString("GitHub release checksum asset \"%1\" contains an invalid size").arg(assetName);
                return false;
            }

            m_checksumAssetName = assetName;
            m_checksumAssetUrl = downloadUrl;
            m_checksumAssetSize = static_cast<qint64>(asset.value("size").toDouble());
            break;
        }
    }

    return true;
}

bool UpdateReader::isUpdateAvailable() const
{
    if (m_currentVersion.isNull() || m_latestVersion.isNull())
        return false;

    return compareVersions(m_currentVersion, m_latestVersion) < 0;
}

QString UpdateReader::normalizedVersionTag(const QString& tag)
{
    QString version = tag.trimmed();
    if (version.startsWith('v', Qt::CaseInsensitive))
        version.remove(0, 1);
    return version;
}

bool UpdateReader::parseDottedVersion(const QString& versionText, QVersionNumber* version, QString* error)
{
    QString text = normalizedVersionTag(versionText);
    static const QRegularExpression versionPattern("^[0-9]+(\\.[0-9]+){2,3}$");

    if (!versionPattern.match(text).hasMatch()) {
        if (error)
            *error = "expected a dotted numeric version with 3 or 4 components";
        return false;
    }

    qsizetype suffixIndex = 0;
    QVersionNumber parsed = QVersionNumber::fromString(text, &suffixIndex);
    if (parsed.isNull() || suffixIndex != text.size()) {
        if (error)
            *error = "could not parse version number";
        return false;
    }

    if (version)
        *version = parsed;
    if (error)
        error->clear();
    return true;
}

bool UpdateReader::parseSha256Checksum(const QByteArray& data, const QString& expectedFileName, QByteArray* checksum, QString* error)
{
    if (checksum)
        checksum->clear();

    QString text = QString::fromUtf8(data).trimmed();
    QString firstLine = text.section('\n', 0, 0).trimmed();
    static const QRegularExpression checksumPattern("^([0-9a-fA-F]{64})(?:\\s+\\*?(.+))?$");
    QRegularExpressionMatch match = checksumPattern.match(firstLine);
    if (!match.hasMatch()) {
        if (error)
            *error = "checksum file does not contain a valid SHA-256 line";
        return false;
    }

    QString fileName = match.captured(2).trimmed();
    if (!fileName.isEmpty() && fileName != expectedFileName) {
        if (error)
            *error = QString("checksum file is for %1 instead of %2").arg(fileName, expectedFileName);
        return false;
    }

    if (checksum)
        *checksum = match.captured(1).toLatin1().toLower();
    if (error)
        error->clear();
    return true;
}

bool UpdateReader::isCurrentPlatformInstallerAsset(const QString& assetName, const QString& versionText)
{
    QString name = assetName.trimmed().toLower();
    QString version = normalizedVersionTag(versionText).toLower();
    if (version.isEmpty())
        return false;

#if defined(Q_OS_WIN)
    QString expectedName = QString("tonatiuhpp-%1-windows-x64.exe").arg(version);
    return name == expectedName;
#else
    Q_UNUSED(name);
    Q_UNUSED(version);
    return false;
#endif
}
