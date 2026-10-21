#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>

#include <Inventor/nodes/SoGroup.h>

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QJSEngine>
#include <QJSValue>
#include <QStringList>

#include "core/CorePluginRegistry.h"
#include "core/TonatiuhCore.h"
#include "kernel/scene/TSceneKit.h"
#include "kernel/scene/TSeparatorKit.h"
#include "kernel/sun/SunPosition.h"
#include "script/ScriptHost.h"
#include "script/ScriptSceneDocument.h"
#include "script/ScriptSceneServices.h"

namespace
{
QString exampleDirectory()
{
    return QDir(QString::fromUtf8(TONATIUHPP_LEGACY_SCRIPT_EXAMPLES_ROOT)).absolutePath();
}

QString readScript(const QString& fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return {};
    return QString::fromUtf8(file.readAll());
}

struct LegacySceneFixture
{
    CorePluginRegistry factories;
    ScriptHost host;
    ScriptSceneDocument document;

    LegacySceneFixture()
    {
        factories.loadScenePlugins({QString::fromUtf8(TONATIUHPP_SCIENTIFIC_PLUGIN_DIRECTORY)});
        auto* scene = new ScriptSceneServices(&host);
        scene->setSceneProvider([this]() { return document.scene(); });
        scene->setClearDocument([this](QString* error) { return document.clear(error); });
        scene->setInsertNode([this](SoNode* node, QString* error) {
            return document.insert(node, error);
        });
        scene->setNodeCreator([this](ScriptSceneServices::FactoryKind kind,
                                     const QString& name) -> SoNode* {
            switch (kind) {
            case ScriptSceneServices::FactoryKind::Shape:
                return factories.createSceneNode("shape", name);
            case ScriptSceneServices::FactoryKind::Profile:
                return factories.createSceneNode("profile", name);
            case ScriptSceneServices::FactoryKind::Material:
                return factories.createSceneNode("material", name);
            case ScriptSceneServices::FactoryKind::Tracker:
                return factories.createSceneNode("tracker", name);
            case ScriptSceneServices::FactoryKind::Sun:
                return factories.createSceneNode("sun", name);
            case ScriptSceneServices::FactoryKind::Air:
                return factories.createSceneNode("air", name);
            }
            return nullptr;
        });
        registerScriptSceneServices(host.engine(), scene);
        const QJSValue facade = host.engine()->globalObject().property("__tonatiuhppSceneApi");
        host.engine()->globalObject().setProperty("tn", facade);
        host.engine()->globalObject().setProperty("tonatiuh", facade);
    }

    QJSValue runExample(const QString& relativePath)
    {
        const QString fileName = QDir(exampleDirectory()).filePath(relativePath);
        return host.evaluate(readScript(fileName), fileName);
    }

    void expectCpcScene(int cpcChildren)
    {
        ASSERT_NE(document.scene(), nullptr);
        TSeparatorKit* layout = document.scene()->getLayout();
        ASSERT_NE(layout, nullptr);
        auto* layoutGroup = dynamic_cast<SoGroup*>(layout->getPart("group", false));
        ASSERT_NE(layoutGroup, nullptr);
        ASSERT_EQ(layoutGroup->getNumChildren(), 1);

        auto* root = dynamic_cast<TSeparatorKit*>(layoutGroup->getChild(0));
        ASSERT_NE(root, nullptr);
        EXPECT_STREQ(root->getName().getString(), "Script");
        auto* rootGroup = dynamic_cast<SoGroup*>(root->getPart("group", false));
        ASSERT_NE(rootGroup, nullptr);
        ASSERT_EQ(rootGroup->getNumChildren(), 1);
        auto* cpc = dynamic_cast<TSeparatorKit*>(rootGroup->getChild(0));
        ASSERT_NE(cpc, nullptr);
        EXPECT_STREQ(cpc->getName().getString(), "CPC");
        auto* cpcGroup = dynamic_cast<SoGroup*>(cpc->getPart("group", false));
        ASSERT_NE(cpcGroup, nullptr);
        EXPECT_EQ(cpcGroup->getNumChildren(), cpcChildren);
        auto* sun = dynamic_cast<SunPosition*>(
            document.scene()->getPart("world.sun.position", false));
        ASSERT_NE(sun, nullptr);
        EXPECT_DOUBLE_EQ(sun->azimuth.getValue(), 180.0);
        EXPECT_DOUBLE_EQ(sun->elevation.getValue(), 80.0);
    }
};
} // namespace

TEST(ScientificScriptLegacyCorpus, AllNineteenTrackedScriptsCompileWithoutRunning)
{
    const QString root = exampleDirectory();
    ASSERT_TRUE(QDir(root).isReadable()) << root.toStdString();

    QStringList scripts;
    QDirIterator it(root, {"*.tnhpps"}, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext())
        scripts << it.next();
    std::sort(scripts.begin(), scripts.end());
    ASSERT_EQ(scripts.size(), 19) << "Review the M3.2 sample-script inventory when examples change.";

    // Function(source) parses the exact tracked script without executing its
    // loops, file writes or interception runs (some request >= 1e6 rays).
    ScriptHost host;
    QJSValue function = host.engine()->globalObject().property("Function");
    ASSERT_TRUE(function.isCallable());
    for (const QString& fileName : scripts) {
        const QString source = readScript(fileName);
        ASSERT_FALSE(source.isEmpty()) << fileName.toStdString();
        const QJSValue compiled = function.callAsConstructor({QJSValue(source)});
        EXPECT_FALSE(compiled.isError()) << fileName.toStdString() << ": "
                                         << compiled.toString().toStdString();
    }
}

TEST(ScientificScriptLegacyCorpus, CurvedCpcSampleBuildsUnmodifiedInMemoryScene)
{
    TonatiuhCore::initializeCoin();
    LegacySceneFixture fixture;
    const QJSValue result = fixture.runExample("devices/CPC/CPC-rotated.tnhpps");
    ASSERT_FALSE(result.isError()) << result.toString().toStdString();
    fixture.expectCpcScene(2); // mirror and absorber
}

TEST(ScientificScriptLegacyCorpus, FacetedCpcSampleBuildsUnmodifiedInMemoryScene)
{
    TonatiuhCore::initializeCoin();
    LegacySceneFixture fixture;
    const QJSValue result = fixture.runExample("devices/CPC/CPC-rotated-step.tnhpps");
    ASSERT_FALSE(result.isError()) << result.toString().toStdString();
    fixture.expectCpcScene(8); // six sides, absorber and cap
}

TEST(ScientificScriptLegacyCorpus, TrackerPartRejectsWrongNodeTypeAndAcceptsArmature)
{
    TonatiuhCore::initializeCoin();
    LegacySceneFixture fixture;
    const QJSValue invalid = fixture.host.evaluate(
        "var root = new NodeObject;\n"
        "var tracker = root.createTracker();\n"
        "tracker.setPart('armature', root);\n", "wrong-armature.tnhpps");
    ASSERT_TRUE(invalid.isError());
    EXPECT_EQ(invalid.property("lineNumber").toInt(), 3);
    EXPECT_TRUE(invalid.toString().contains("tracker armature NodeObject"))
        << invalid.toString().toStdString();

    const QJSValue valid = fixture.host.evaluate(
        "var armature = tracker.insertArmature('two-axes');\n"
        "tracker.setPart('armature', armature);\n"
        "tracker.getPart('armature') !== null;\n", "valid-armature.tnhpps");
    ASSERT_FALSE(valid.isError()) << valid.toString().toStdString();
    EXPECT_TRUE(valid.toBool());
}

TEST(ScientificScriptInterception, InvalidInputsKeepCallSite)
{
    TonatiuhCore::initializeCoin();
    LegacySceneFixture fixture;
    const QStringList cases = {
        "tn.FindInterception('', 16);",
        "tn.FindInterception('//Node/Receiver/Shape', 0);",
        "tn.FindInterception('//Node/Receiver/Shape', -1);",
        "tn.FindInterception('//Node/Receiver/Shape', 1.5);",
        "tn.FindInterception('//Node/Receiver/Shape', Infinity);",
        "tn.FindInterception('//Node/Receiver/Shape', '100');",
    };
    for (const QString& code : cases) {
        const QJSValue failure = fixture.host.evaluate(code, "invalid-interception.tnhpps");
        ASSERT_TRUE(failure.isError()) << code.toStdString();
        EXPECT_EQ(failure.property("lineNumber").toInt(), 1) << code.toStdString();
        EXPECT_TRUE(failure.toString().contains("tn.FindInterception"));
    }
}

TEST(ScientificScriptInterception, UnknownSurfaceFailsBeforeTracing)
{
    TonatiuhCore::initializeCoin();
    LegacySceneFixture fixture;
    const QJSValue failure = fixture.host.evaluate(
        "tn.FindInterception('//Node/Nonexistent/Shape', 64);",
        "missing-interception-target.tnhpps");
    ASSERT_TRUE(failure.isError());
    EXPECT_EQ(failure.property("lineNumber").toInt(), 1);
    EXPECT_TRUE(failure.toString().contains("no shape"))
        << failure.toString().toStdString();
}

TEST(ScientificScriptInterception, ShortTraceRepeatsWithFixedHeadlessSeed)
{
    TonatiuhCore::initializeCoin();
    LegacySceneFixture fixture;
    const QString fileName = QString::fromUtf8(TONATIUHPP_SCRIPT_INTERCEPTION_FIXTURE_FILE);
    const QString source = readScript(fileName);
    ASSERT_FALSE(source.isEmpty()) << fileName.toStdString();
    const QJSValue firstRun = fixture.host.evaluate(source, fileName);
    ASSERT_FALSE(firstRun.isError()) << firstRun.toString().toStdString();
    const QJSValue firstPower = fixture.host.engine()->globalObject().property("power");
    ASSERT_TRUE(firstPower.isNumber());
    ASSERT_TRUE(std::isfinite(firstPower.toNumber()));
    EXPECT_GT(firstPower.toNumber(), 0.);

    const QJSValue secondPower = fixture.host.evaluate(
        "tn.FindInterception(url, 2048);", "repeat-interception.tnhpps");
    ASSERT_FALSE(secondPower.isError()) << secondPower.toString().toStdString();
    EXPECT_DOUBLE_EQ(firstPower.toNumber(), secondPower.toNumber());
}
