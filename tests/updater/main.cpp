#include "UpdateReader.h"

#include <QCoreApplication>
#include <QUrl>
#include <QVersionNumber>

#include <iostream>

namespace
{
int failures = 0;

void check(bool condition, const QString& message)
{
    if (condition)
        return;

    ++failures;
    std::cerr << message.toStdString() << '\n';
}

void checkVersion(const QString& input, const QString& expected)
{
    QVersionNumber version;
    QString error;
    bool ok = UpdateReader::parseDottedVersion(input, &version, &error);
    check(ok, QString("Expected version to parse: %1").arg(input));
    if (ok)
        check(version.toString() == expected, QString("Unexpected normalized version for %1").arg(input));
}

void checkInvalidVersion(const QString& input)
{
    QVersionNumber version;
    QString error;
    check(!UpdateReader::parseDottedVersion(input, &version, &error), QString("Expected version to fail: %1").arg(input));
    check(!error.isEmpty(), QString("Expected error message for version: %1").arg(input));
}

void checkChecksum(const QByteArray& input, const QString& expectedFileName, const QByteArray& expectedChecksum)
{
    QByteArray checksum;
    QString error;
    bool ok = UpdateReader::parseSha256Checksum(input, expectedFileName, &checksum, &error);
    check(ok, QString("Expected checksum to parse for %1").arg(expectedFileName));
    if (ok)
        check(checksum == expectedChecksum, QString("Unexpected checksum for %1").arg(expectedFileName));
}

void checkInvalidChecksum(const QByteArray& input, const QString& expectedFileName)
{
    QByteArray checksum;
    QString error;
    check(!UpdateReader::parseSha256Checksum(input, expectedFileName, &checksum, &error), "Expected checksum to fail");
    check(!error.isEmpty(), "Expected checksum error message");
    check(checksum.isEmpty(), "Expected failed checksum parse to clear output");
}

QString expectedPlatformAssetName()
{
#if defined(Q_OS_WIN)
    return "TonatiuhPP-1.0.1-windows-x64.exe";
#else
    return QString();
#endif
}
}

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationVersion("1.0.0");

    checkVersion("v1.2.3", "1.2.3");
    checkVersion("1.2.3.4", "1.2.3.4");
    checkVersion("  V0.9.0  ", "0.9.0");

    checkInvalidVersion("1.2");
    checkInvalidVersion("1.2.3.4.5");
    checkInvalidVersion("v1.2.beta");

    QByteArray sampleHash = "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";
    checkChecksum(sampleHash + "  TonatiuhPP-1.0.1-windows-x64.exe\n", "TonatiuhPP-1.0.1-windows-x64.exe", sampleHash);
    checkChecksum(sampleHash.toUpper() + " *tonatiuhpp-Linux.tar.gz\n", "tonatiuhpp-Linux.tar.gz", sampleHash);
    checkInvalidChecksum("not-a-checksum  TonatiuhPP-1.0.1-windows-x64.exe\n", "TonatiuhPP-1.0.1-windows-x64.exe");
    checkInvalidChecksum(sampleHash + "  other-file.exe\n", "TonatiuhPP-1.0.1-windows-x64.exe");

    check(!UpdateReader::isCurrentPlatformInstallerAsset("tonatiuhpp-Linux.tar.gz", "1.0.1"), "Expected Linux archive to be ignored by the self-update asset matcher");
    check(!UpdateReader::isCurrentPlatformInstallerAsset("tonatiuhpp-macOS.tar.gz", "1.0.1"), "Expected macOS archive to be ignored by the self-update asset matcher");
    check(!UpdateReader::isCurrentPlatformInstallerAsset("TonatiuhPP-1.0.0-windows-x64.exe", "1.0.1"), "Expected stale installer asset to be ignored");
#if defined(Q_OS_WIN)
    check(UpdateReader::isCurrentPlatformInstallerAsset("TonatiuhPP-1.0.1-windows-x64.exe", "v1.0.1"), "Expected matching Windows installer asset to be accepted");
#endif

    UpdateReader newer;
    check(
        newer.readGitHubRelease(R"({"tag_name":"v1.0.1","html_url":"https://github.com/CST-Modelling-Tools/tonatiuhpp/releases/tag/v1.0.1"})"),
        "Expected newer GitHub release JSON to parse"
    );
    check(newer.isUpdateAvailable(), "Expected v1.0.1 to be newer than 1.0.0");
    check(newer.releaseUrl() == QUrl("https://github.com/CST-Modelling-Tools/tonatiuhpp/releases/tag/v1.0.1"), "Unexpected release URL");

    UpdateReader releaseWithAssets;
    check(
        releaseWithAssets.readGitHubRelease(R"({
            "tag_name":"v1.0.1",
            "html_url":"https://github.com/CST-Modelling-Tools/tonatiuhpp/releases/tag/v1.0.1",
            "assets":[
                {"name":"tonatiuhpp-Linux.tar.gz","browser_download_url":"https://github.com/CST-Modelling-Tools/tonatiuhpp/releases/download/v1.0.1/tonatiuhpp-Linux.tar.gz","size":101},
                {"name":"tonatiuhpp-Linux.tar.gz.sha256","browser_download_url":"https://github.com/CST-Modelling-Tools/tonatiuhpp/releases/download/v1.0.1/tonatiuhpp-Linux.tar.gz.sha256","size":90},
                {"name":"tonatiuhpp-macOS.tar.gz","browser_download_url":"https://github.com/CST-Modelling-Tools/tonatiuhpp/releases/download/v1.0.1/tonatiuhpp-macOS.tar.gz","size":102},
                {"name":"tonatiuhpp-macOS.tar.gz.sha256","browser_download_url":"https://github.com/CST-Modelling-Tools/tonatiuhpp/releases/download/v1.0.1/tonatiuhpp-macOS.tar.gz.sha256","size":90},
                {"name":"TonatiuhPP-1.0.0-windows-x64.exe","browser_download_url":"https://github.com/CST-Modelling-Tools/tonatiuhpp/releases/download/v1.0.1/TonatiuhPP-1.0.0-windows-x64.exe","size":99},
                {"name":"TonatiuhPP-1.0.1-windows-x64.exe","browser_download_url":"https://github.com/CST-Modelling-Tools/tonatiuhpp/releases/download/v1.0.1/TonatiuhPP-1.0.1-windows-x64.exe","size":103},
                {"name":"TonatiuhPP-1.0.1-windows-x64.exe.sha256","browser_download_url":"https://github.com/CST-Modelling-Tools/tonatiuhpp/releases/download/v1.0.1/TonatiuhPP-1.0.1-windows-x64.exe.sha256","size":90}
            ]
        })"),
        "Expected GitHub release JSON with assets to parse"
    );
    QString expectedAsset = expectedPlatformAssetName();
    if (!expectedAsset.isEmpty()) {
        check(releaseWithAssets.hasDownloadAsset(), "Expected a download asset for the current platform");
        check(releaseWithAssets.downloadAssetName() == expectedAsset, "Unexpected platform download asset name");
        check(
            releaseWithAssets.downloadAssetUrl() == QUrl(QString("https://github.com/CST-Modelling-Tools/tonatiuhpp/releases/download/v1.0.1/%1").arg(expectedAsset)),
            "Unexpected platform download asset URL"
        );
        check(releaseWithAssets.downloadAssetSize() > 0, "Expected platform download asset size");
        check(releaseWithAssets.hasChecksumAsset(), "Expected a checksum asset for the current platform");
        check(releaseWithAssets.checksumAssetName() == QString("%1.sha256").arg(expectedAsset), "Unexpected platform checksum asset name");
        check(
            releaseWithAssets.checksumAssetUrl() == QUrl(QString("https://github.com/CST-Modelling-Tools/tonatiuhpp/releases/download/v1.0.1/%1.sha256").arg(expectedAsset)),
            "Unexpected platform checksum asset URL"
        );
    } else {
        check(!releaseWithAssets.hasDownloadAsset(), "Expected no download asset on unsupported platforms");
        check(!releaseWithAssets.hasChecksumAsset(), "Expected no checksum asset on unsupported platforms");
    }

    UpdateReader same;
    check(
        same.readGitHubRelease(R"({"tag_name":"v1.0.0","html_url":"https://github.com/CST-Modelling-Tools/tonatiuhpp/releases/tag/v1.0.0"})"),
        "Expected same-version GitHub release JSON to parse"
    );
    check(!same.isUpdateAvailable(), "Expected v1.0.0 not to be newer than 1.0.0");

    QCoreApplication::setApplicationVersion("1.0.0.0");
    UpdateReader sameWithTrailingZero;
    check(
        sameWithTrailingZero.readGitHubRelease(R"({"tag_name":"v1.0.0","html_url":"https://github.com/CST-Modelling-Tools/tonatiuhpp/releases/tag/v1.0.0"})"),
        "Expected three-component release JSON to parse"
    );
    check(!sameWithTrailingZero.isUpdateAvailable(), "Expected v1.0.0 not to be newer than 1.0.0.0");

    QCoreApplication::setApplicationVersion("1.0.0");
    UpdateReader newerFourthComponent;
    check(
        newerFourthComponent.readGitHubRelease(R"({"tag_name":"v1.0.0.1","html_url":"https://github.com/CST-Modelling-Tools/tonatiuhpp/releases/tag/v1.0.0.1"})"),
        "Expected four-component release JSON to parse"
    );
    check(newerFourthComponent.isUpdateAvailable(), "Expected v1.0.0.1 to be newer than 1.0.0");

    UpdateReader malformedTag;
    check(
        !malformedTag.readGitHubRelease(R"({"tag_name":"v1.0","html_url":"https://github.com/CST-Modelling-Tools/tonatiuhpp/releases/tag/v1.0"})"),
        "Expected malformed tag to fail"
    );
    check(!malformedTag.errorMessage().isEmpty(), "Expected malformed tag to set an error message");

    UpdateReader missingUrl;
    check(
        !missingUrl.readGitHubRelease(R"({"tag_name":"v1.0.1"})"),
        "Expected missing html_url to fail"
    );
    check(!missingUrl.errorMessage().isEmpty(), "Expected missing html_url to set an error message");

    UpdateReader insecureUrl;
    check(
        !insecureUrl.readGitHubRelease(R"({"tag_name":"v1.0.1","html_url":"http://github.com/CST-Modelling-Tools/tonatiuhpp/releases/tag/v1.0.1"})"),
        "Expected non-HTTPS html_url to fail"
    );
    check(!insecureUrl.errorMessage().isEmpty(), "Expected non-HTTPS html_url to set an error message");

    UpdateReader wrongHostUrl;
    check(
        !wrongHostUrl.readGitHubRelease(R"({"tag_name":"v1.0.1","html_url":"https://example.com/CST-Modelling-Tools/tonatiuhpp/releases/tag/v1.0.1"})"),
        "Expected non-GitHub html_url to fail"
    );
    check(!wrongHostUrl.errorMessage().isEmpty(), "Expected non-GitHub html_url to set an error message");

    if (!expectedAsset.isEmpty()) {
        UpdateReader badAssetUrl;
        QString badAssetJson = QString(R"({
            "tag_name":"v1.0.1",
            "html_url":"https://github.com/CST-Modelling-Tools/tonatiuhpp/releases/tag/v1.0.1",
            "assets":[
                {"name":"%1","browser_download_url":"https://example.com/downloads/%1","size":10}
            ]
        })").arg(expectedAsset);
        check(!badAssetUrl.readGitHubRelease(badAssetJson.toUtf8()), "Expected unexpected download asset URL to fail");
        check(!badAssetUrl.errorMessage().isEmpty(), "Expected unexpected download asset URL to set an error message");

        UpdateReader badChecksumUrl;
        QString badChecksumJson = QString(R"({
            "tag_name":"v1.0.1",
            "html_url":"https://github.com/CST-Modelling-Tools/tonatiuhpp/releases/tag/v1.0.1",
            "assets":[
                {"name":"%1","browser_download_url":"https://github.com/CST-Modelling-Tools/tonatiuhpp/releases/download/v1.0.1/%1","size":10},
                {"name":"%1.sha256","browser_download_url":"https://example.com/downloads/%1.sha256","size":10}
            ]
        })").arg(expectedAsset);
        check(!badChecksumUrl.readGitHubRelease(badChecksumJson.toUtf8()), "Expected unexpected checksum asset URL to fail");
        check(!badChecksumUrl.errorMessage().isEmpty(), "Expected unexpected checksum asset URL to set an error message");
    }

    return failures == 0 ? 0 : 1;
}
