#include "UpdateReader.h"

#include <QCoreApplication>
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
}

UpdateReader::UpdateReader()
{
    m_currentVersionText = QCoreApplication::applicationVersion();
    parseDottedVersion(m_currentVersionText, &m_currentVersion, &m_currentVersionError);
}

bool UpdateReader::readGitHubRelease(const QByteArray& data)
{
    m_message.clear();
    m_latestTagName.clear();
    m_latestVersionText.clear();
    m_latestVersion = QVersionNumber();
    m_releaseUrl = QUrl();

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
    if (m_releaseUrl.host().compare("github.com", Qt::CaseInsensitive) != 0 ||
        !m_releaseUrl.path().startsWith("/CST-Modelling-Tools/tonatiuhpp/releases/", Qt::CaseInsensitive)) {
        m_message = QString("GitHub release response contains an unexpected html_url: %1").arg(urlValue.toString());
        return false;
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
