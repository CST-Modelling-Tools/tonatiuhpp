#include "application/core/ApplicationDataPaths.h"

#include <gtest/gtest.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>

TEST(ScientificAppDataPaths, FindsPackagedExamplesAndScripts)
{
    QTemporaryDir temporary;
    ASSERT_TRUE(temporary.isValid());
    QDir root(temporary.path());
#ifdef Q_OS_MACOS
    const QString prefix = QStringLiteral("installed/TonatiuhPP.app/Contents/Resources");
    const QString appRelative = QStringLiteral("installed/TonatiuhPP.app/Contents/MacOS");
#else
    const QString prefix = QStringLiteral("installed");
    const QString appRelative = QStringLiteral("installed/bin");
#endif
    ASSERT_TRUE(root.mkpath(appRelative));
    ASSERT_TRUE(root.mkpath(prefix + "/examples/scripts"));
    ASSERT_TRUE(root.mkpath(prefix + "/resources"));

    const QString app = root.filePath(appRelative);
    EXPECT_EQ(TonatiuhppAppDataPaths::findDataDirectory("examples", app),
              QFileInfo(root.filePath(prefix + "/examples")).canonicalFilePath());
    EXPECT_EQ(TonatiuhppAppDataPaths::findDataDirectory("examples/scripts", app),
              QFileInfo(root.filePath(prefix + "/examples/scripts")).canonicalFilePath());
    EXPECT_EQ(TonatiuhppAppDataPaths::findDataDirectory("resources", app),
              QFileInfo(root.filePath(prefix + "/resources")).canonicalFilePath());
}

TEST(ScientificAppDataPaths, FindsSourcesFromCMakeCacheWithSeparateBuildTree)
{
    QTemporaryDir temporary;
    ASSERT_TRUE(temporary.isValid());
    QDir root(temporary.path());
    ASSERT_TRUE(root.mkpath("checkout/source"));
    ASSERT_TRUE(root.mkpath("checkout/examples/scripts"));
    ASSERT_TRUE(root.mkpath("checkout/resources"));
    ASSERT_TRUE(root.mkpath("out/build/bin/Release"));

    QFile cache(root.filePath("out/build/CMakeCache.txt"));
    ASSERT_TRUE(cache.open(QIODevice::WriteOnly | QIODevice::Text));
    const QByteArray content = QByteArray("CMAKE_HOME_DIRECTORY:INTERNAL=")
        + QDir(root.filePath("checkout/source")).absolutePath().toUtf8() + '\n';
    ASSERT_EQ(cache.write(content), content.size());
    cache.close();

    const QString app = root.filePath("out/build/bin/Release");
    EXPECT_EQ(TonatiuhppAppDataPaths::findDataDirectory("examples", app),
              QFileInfo(root.filePath("checkout/examples")).canonicalFilePath());
    EXPECT_EQ(TonatiuhppAppDataPaths::findDataDirectory("examples/scripts", app),
              QFileInfo(root.filePath("checkout/examples/scripts")).canonicalFilePath());
    EXPECT_EQ(TonatiuhppAppDataPaths::findDataDirectory("resources", app),
              QFileInfo(root.filePath("checkout/resources")).canonicalFilePath());
}

TEST(ScientificAppDataPaths, PackagedDataTakesPriorityOverDeveloperSources)
{
    QTemporaryDir temporary;
    ASSERT_TRUE(temporary.isValid());
    QDir root(temporary.path());
    ASSERT_TRUE(root.mkpath("checkout/source"));
    ASSERT_TRUE(root.mkpath("checkout/examples"));
#ifdef Q_OS_MACOS
    const QString appRelative = QStringLiteral("checkout/build/bin/TonatiuhPP.app/Contents/MacOS");
    const QString packagedRelative = QStringLiteral("checkout/build/bin/TonatiuhPP.app/Contents/Resources/examples");
#else
    const QString appRelative = QStringLiteral("checkout/build/bin/Release");
    const QString packagedRelative = QStringLiteral("checkout/build/bin/examples");
#endif
    ASSERT_TRUE(root.mkpath(appRelative));
    ASSERT_TRUE(root.mkpath(packagedRelative));
    QFile cache(root.filePath("checkout/build/CMakeCache.txt"));
    ASSERT_TRUE(cache.open(QIODevice::WriteOnly | QIODevice::Text));
    const QByteArray content = QByteArray("CMAKE_HOME_DIRECTORY:INTERNAL=")
        + QDir(root.filePath("checkout/source")).absolutePath().toUtf8() + '\n';
    ASSERT_EQ(cache.write(content), content.size());
    cache.close();
    EXPECT_EQ(TonatiuhppAppDataPaths::findDataDirectory("examples",
                  root.filePath(appRelative)),
              QFileInfo(root.filePath(packagedRelative)).canonicalFilePath());
}

TEST(ScientificAppDataPaths, MissingDirectoryReturnsEmptyInsteadOfDocuments)
{
    QTemporaryDir temporary;
    ASSERT_TRUE(temporary.isValid());
    QDir root(temporary.path());
    ASSERT_TRUE(root.mkpath("isolated/bin/Release"));
    const QString app = root.filePath("isolated/bin/Release");
    EXPECT_TRUE(TonatiuhppAppDataPaths::findDataDirectory("examples", app).isEmpty());
    EXPECT_TRUE(TonatiuhppAppDataPaths::findDataDirectory("examples/scripts", app).isEmpty());
    EXPECT_TRUE(TonatiuhppAppDataPaths::findDataDirectory("../other", app).isEmpty());
}

TEST(ScientificAppDataPaths, FindsPackagedHtmlDocumentation)
{
    QTemporaryDir temporary;
    ASSERT_TRUE(temporary.isValid());
    QDir root(temporary.path());
#ifdef Q_OS_MACOS
    const QString appRelative = QStringLiteral("installed/TonatiuhPP.app/Contents/MacOS");
    const QString helpRelative = QStringLiteral("installed/TonatiuhPP.app/Contents/Resources/help/html");
#else
    const QString appRelative = QStringLiteral("installed/bin");
    const QString helpRelative = QStringLiteral("installed/help/html");
#endif
    ASSERT_TRUE(root.mkpath(appRelative));
    ASSERT_TRUE(root.mkpath(helpRelative));
    QFile index(root.filePath(helpRelative + "/index.html"));
    ASSERT_TRUE(index.open(QIODevice::WriteOnly));
    ASSERT_NE(index.write("<html>help</html>"), -1);
    index.close();

    EXPECT_EQ(TonatiuhppAppDataPaths::findDataFile("help/html/index.html",
                  root.filePath(appRelative)),
              QFileInfo(index.fileName()).canonicalFilePath());
}

TEST(ScientificAppDataPaths, FindsHtmlDocumentationFromSeparateCMakeBuild)
{
    QTemporaryDir temporary;
    ASSERT_TRUE(temporary.isValid());
    QDir root(temporary.path());
    ASSERT_TRUE(root.mkpath("checkout/source"));
    ASSERT_TRUE(root.mkpath("checkout/help/html"));
    ASSERT_TRUE(root.mkpath("out/build/bin/Release"));
    QFile index(root.filePath("checkout/help/html/index.html"));
    ASSERT_TRUE(index.open(QIODevice::WriteOnly));
    ASSERT_NE(index.write("<html>help</html>"), -1);
    index.close();

    QFile cache(root.filePath("out/build/CMakeCache.txt"));
    ASSERT_TRUE(cache.open(QIODevice::WriteOnly | QIODevice::Text));
    const QByteArray entry = QByteArray("CMAKE_HOME_DIRECTORY:INTERNAL=")
        + QDir(root.filePath("checkout/source")).absolutePath().toUtf8() + '\n';
    ASSERT_EQ(cache.write(entry), entry.size());
    cache.close();

    EXPECT_EQ(TonatiuhppAppDataPaths::findDataFile("help/html/index.html",
                  root.filePath("out/build/bin/Release")),
              QFileInfo(index.fileName()).canonicalFilePath());
}

TEST(ScientificAppDataPaths, MissingHelpIndexReturnsEmpty)
{
    QTemporaryDir temporary;
    ASSERT_TRUE(temporary.isValid());
    QDir root(temporary.path());
    ASSERT_TRUE(root.mkpath("installed/bin"));
    ASSERT_TRUE(root.mkpath("installed/help/html")); // directory exists; HTML does not
    EXPECT_TRUE(TonatiuhppAppDataPaths::findDataFile("help/html/index.html",
                root.filePath("installed/bin")).isEmpty());
    EXPECT_TRUE(TonatiuhppAppDataPaths::findDataFile("../secret.txt",
                root.filePath("installed/bin")).isEmpty());
}
