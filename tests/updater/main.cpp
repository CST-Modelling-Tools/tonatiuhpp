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

    UpdateReader newer;
    check(
        newer.readGitHubRelease(R"({"tag_name":"v1.0.1","html_url":"https://github.com/CST-Modelling-Tools/tonatiuhpp/releases/tag/v1.0.1"})"),
        "Expected newer GitHub release JSON to parse"
    );
    check(newer.isUpdateAvailable(), "Expected v1.0.1 to be newer than 1.0.0");
    check(newer.releaseUrl() == QUrl("https://github.com/CST-Modelling-Tools/tonatiuhpp/releases/tag/v1.0.1"), "Unexpected release URL");

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

    return failures == 0 ? 0 : 1;
}
