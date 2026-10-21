#include <gtest/gtest.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJSValue>
#include <QTemporaryDir>

#include <QStringList>

#include "script/ScriptHost.h"

TEST(ScientificScriptHost, IndependentEnginesAndDataArrays)
{
    ScriptHost first;
    ScriptHost second;

    QJSValue firstValue = first.evaluate(
        "var data = new DataObject; data.addArray(['first', '1']);"
        "data.array(0).join(':');", "/tmp/first.tnhpps");
    QJSValue secondValue = second.evaluate(
        "var data = new DataObject; data.addArray(['second', '2']);"
        "data.array(0).join(':');", "/tmp/second.tnhpps");
    ASSERT_FALSE(firstValue.isError()) << firstValue.toString().toStdString();
    ASSERT_FALSE(secondValue.isError()) << secondValue.toString().toStdString();
    EXPECT_EQ(firstValue.toString(), "first:1");
    EXPECT_EQ(secondValue.toString(), "second:2");

    const QJSValue later = first.evaluate("data.array(0).join(':');");
    ASSERT_FALSE(later.isError()) << later.toString().toStdString();
    EXPECT_EQ(later.toString(), "first:1");
}

TEST(ScientificScriptHost, DataFilesResolveFromScriptDirectory)
{
    QTemporaryDir directory;
    ASSERT_TRUE(directory.isValid());
    const QString scriptName = QDir(directory.path()).filePath("example.tnhpps");

    ScriptHost host;
    const QJSValue written = host.evaluate(
        "var data = new DataObject;"
        "data.addRow('name,value');"
        "data.addArray(['mirror', '42']);"
        "data.write('measurements.csv');", scriptName);
    ASSERT_FALSE(written.isError()) << written.toString().toStdString();
    ASSERT_TRUE(written.toBool());

    const QString expectedPath = QDir(directory.path()).filePath("measurements.csv");
    EXPECT_TRUE(QFileInfo::exists(expectedPath));

    ScriptHost second;
    const QJSValue readBack = second.evaluate(
        "var copy = new DataObject;"
        "if (!copy.read('measurements.csv')) throw new Error('read failed');"
        "copy.rows() + ':' + copy.array(1).join('/');", scriptName);
    ASSERT_FALSE(readBack.isError()) << readBack.toString().toStdString();
    EXPECT_EQ(readBack.toString(), "2:mirror/42");
}

TEST(ScientificScriptHost, ReportsScriptFileAndLineFromCppErrors)
{
    QTemporaryDir directory;
    ASSERT_TRUE(directory.isValid());
    const QString filename = QDir(directory.path()).filePath("missing-file.tnhpps");
    ScriptHost host;
    const QJSValue error = host.evaluate(
        "var data = new DataObject;\n"
        "data.read('missing.csv');\n", filename);
    ASSERT_TRUE(error.isError());
    EXPECT_EQ(error.property("lineNumber").toInt(), 2);
    const QString message = ScriptHost::formatError(error, filename);
    EXPECT_TRUE(message.contains("missing-file.tnhpps:2")) << message.toStdString();
    EXPECT_TRUE(message.contains("DataObject.read cannot open")) << message.toStdString();
}

TEST(ScientificScriptHost, ReportsSyntaxErrorsAndSupportsIndependentLogging)
{
    ScriptHost first;
    ScriptHost second;
    QStringList firstMessages;
    QStringList secondMessages;
    first.setPrintCallback([&firstMessages](const QString& message) { firstMessages << message; });
    second.setPrintCallback([&secondMessages](const QString& message) { secondMessages << message; });

    EXPECT_FALSE(first.evaluate("print('first'); printTimed('timed');").isError());
    EXPECT_FALSE(second.evaluate("print('second');").isError());
    ASSERT_EQ(firstMessages.size(), 2);
    ASSERT_EQ(secondMessages.size(), 1);
    EXPECT_EQ(firstMessages.at(0), "first");
    EXPECT_TRUE(firstMessages.at(1).endsWith("timed"));
    EXPECT_EQ(secondMessages.at(0), "second");

    EXPECT_EQ(ScriptHost::formatError(QJSValue("plain error"), "fallback.tnhpps"),
              "fallback.tnhpps: plain error");
    const QJSValue error = first.evaluate("var n = ;", "broken.tnhpps");
    ASSERT_TRUE(error.isError());
    EXPECT_TRUE(ScriptHost::formatError(error, "broken.tnhpps").contains("broken.tnhpps:1"));
}
