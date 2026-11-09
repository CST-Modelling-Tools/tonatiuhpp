#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <functional>
#include <vector>

#include <Inventor/nodes/SoGroup.h>

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QPluginLoader>
#include <QTemporaryDir>
#include <QCoreApplication>
#include <QFileInfo>
#include <memory>
#include <QJSEngine>
#include <QJSValue>
#include <QStringList>

#include "core/CorePluginRegistry.h"
#include "core/GuiTraceCompletion.h"
#include "core/PhotonExportFileService.h"
#include "core/GuiTraceRunInputs.h"
#include "run/FluxAnalysisTsvWriter.h"
#include "core/FluxGridComputation.h"
#include "headless/HeadlessScriptFluxBackend.h"
#include "core/SceneInstanceBuilder.h"
#include "core/TonatiuhCore.h"
#include "kernel/scene/TSceneKit.h"
#include "kernel/scene/TSeparatorKit.h"
#include "kernel/sun/SunPosition.h"
#include "script/ScriptHost.h"
#include "script/ScriptSceneDocument.h"
#include "script/ScriptSceneServices.h"
#include "script/ScriptRunRequest.h"
#include "script/ScriptFluxAnalysisRequest.h"
#include <QJsonArray>
#include <QJsonDocument>
#include "headless/HeadlessScriptRunFileBackend.h"
#include "script/ScriptRunExecutionService.h"
#include "script/GuiScriptFacadeBridge.h"
#include "kernel/photons/PhotonsAbstract.h"
#include "kernel/photons/PhotonsBuffer.h"

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

TEST(ScientificScriptInterception, HostRejectsInvalidSunGridWithoutTracing)
{
    TonatiuhCore::initializeCoin();
    LegacySceneFixture fixture;
    auto* services = qobject_cast<ScriptSceneServices*>(fixture.host.engine()
        ->globalObject().property("__tonatiuhppSceneApi").toQObject());
    ASSERT_NE(services, nullptr);
    int policiesRequested = 0;
    services->setInterceptionConfiguration([&policiesRequested](ulong rays) {
        ++policiesRequested;
        SimulationConfig config;
        config.rays = rays;
        config.sunWidthDivisions = 0; // Must fail before resolving the surface or tracing.
        return config;
    });
    const QJSValue failure = fixture.host.evaluate(
        "tn.FindInterception('//Node/Absent/Shape', 64);",
        "invalid-script-grid.tnhpps");
    ASSERT_TRUE(failure.isError());
    EXPECT_EQ(failure.property("lineNumber").toInt(), 1);
    EXPECT_TRUE(failure.toString().contains("Sun grid dimensions"))
        << failure.toString().toStdString();
    EXPECT_EQ(policiesRequested, 1);
}

TEST(ScientificScriptInterception, NonDefaultSunGridRepeatsExactly)
{
    TonatiuhCore::initializeCoin();
    LegacySceneFixture fixture;
    auto* services = qobject_cast<ScriptSceneServices*>(fixture.host.engine()
        ->globalObject().property("__tonatiuhppSceneApi").toQObject());
    ASSERT_NE(services, nullptr);
    services->setInterceptionConfiguration([](ulong rays) {
        SimulationConfig config;
        config.rays = rays;
        config.masterSeed = 0;
        config.sunWidthDivisions = 64;
        config.sunHeightDivisions = 48;
        return config;
    });
    const QString fileName = QString::fromUtf8(TONATIUHPP_SCRIPT_INTERCEPTION_FIXTURE_FILE);
    const QJSValue first = fixture.host.evaluate(readScript(fileName), fileName);
    ASSERT_FALSE(first.isError()) << first.toString().toStdString();
    const QJSValue firstPower = fixture.host.engine()->globalObject().property("power");
    ASSERT_TRUE(firstPower.isNumber());
    EXPECT_TRUE(std::isfinite(firstPower.toNumber()));
    EXPECT_GT(firstPower.toNumber(), 0.);
    const QJSValue second = fixture.host.evaluate(
        "tn.FindInterception(url, 2048);", "nondefault-grid-repeat.tnhpps");
    ASSERT_FALSE(second.isError()) << second.toString().toStdString();
    EXPECT_DOUBLE_EQ(firstPower.toNumber(), second.toNumber());
}

TEST(ScientificScriptInterception, HeadlessShowFlagFailsAtCallSite)
{
    TonatiuhCore::initializeCoin();
    LegacySceneFixture fixture;
    const QJSValue failure = fixture.host.evaluate(
        "tn.FindInterception('//Node/Absent/Shape', 64, true);",
        "headless-show-rejected.tnhpps");
    ASSERT_TRUE(failure.isError());
    EXPECT_EQ(failure.property("lineNumber").toInt(), 1);
    EXPECT_TRUE(failure.toString().contains("GUI-only presentation"))
        << failure.toString().toStdString();
}

TEST(ScientificScriptInterception, NodeObjectUsesSharedValidation)
{
    TonatiuhCore::initializeCoin();
    LegacySceneFixture fixture;
    const QJSValue failure = fixture.host.evaluate(
        "var node = new NodeObject; node.FindInterception('//Node/Absent/Shape', -1);",
        "invalid-node-interception.tnhpps");
    ASSERT_TRUE(failure.isError());
    EXPECT_TRUE(failure.toString().contains("positive integer ray count"))
        << failure.toString().toStdString();
}

TEST(ScientificScriptSunPosition, HistoricalAnglesWorkThroughSharedSceneApi)
{
    TonatiuhCore::initializeCoin();
    LegacySceneFixture fixture;
    const QJSValue angles = fixture.host.evaluate(
        "tn.Clear();\n"
        "tonatiuh.ChangeSunPosition(179.5, 63.25);\n"
        "var sun = tn.getScene().getPart('world.sun.position');\n"
        "[Number(sun.getParameter('azimuth')), Number(sun.getParameter('elevation'))];",
        "shared-sun-angles.tnhpps");
    ASSERT_FALSE(angles.isError()) << angles.toString().toStdString();
    EXPECT_DOUBLE_EQ(angles.property(0).toNumber(), 179.5);
    EXPECT_DOUBLE_EQ(angles.property(1).toNumber(), 63.25);
}

TEST(ScientificScriptSunPosition, CalendarLocationOverloadRunsWithoutWidgets)
{
    TonatiuhCore::initializeCoin();
    LegacySceneFixture fixture;
    const QJSValue result = fixture.host.evaluate(
        "tn.Clear();\n"
        "tn.ChangeSunPosition(2020, 6, 21, 12, 0, 0, 40, -3);\n"
        "var pos = tn.getScene().getPart('world.sun.position');\n"
        "var az = Number(pos.getParameter('azimuth'));\n"
        "var el = Number(pos.getParameter('elevation'));\n"
        "isFinite(az) && isFinite(el);",
        "shared-sun-calendar.tnhpps");
    ASSERT_FALSE(result.isError()) << result.toString().toStdString();
    EXPECT_TRUE(result.toBool());
}

TEST(ScientificScriptSunPosition, InvalidCalendarAndNumericAnglesPreserveState)
{
    TonatiuhCore::initializeCoin();
    LegacySceneFixture fixture;
    const QJSValue set = fixture.host.evaluate(
        "tn.Clear(); tn.ChangeSunPosition(179.5, 63.25);", "sun-valid.tnhpps");
    ASSERT_FALSE(set.isError()) << set.toString().toStdString();
    for (const QString& statement : {
             QStringLiteral("tn.ChangeSunPosition(NaN, 10);"),
             QStringLiteral("tn.ChangeSunPosition(2020, 13, 21, 12, 0, 0, 40, -3);"),
             QStringLiteral("tn.ChangeSunPosition(2020, 6, 21, 12, 0, 0, 140, -3);")
         }) {
        const QJSValue invalid = fixture.host.evaluate(statement, "invalid-sun.tnhpps");
        ASSERT_TRUE(invalid.isError()) << statement.toStdString();
        EXPECT_EQ(invalid.property("lineNumber").toInt(), 1);
        EXPECT_TRUE(invalid.toString().contains("tn.ChangeSunPosition"));
        const QJSValue unchanged = fixture.host.evaluate(
            "var sun = tn.getScene().getPart('world.sun.position');"
            "Number(sun.getParameter('azimuth')) === 179.5 && "
            "Number(sun.getParameter('elevation')) === 63.25;",
            "sun-preserved.tnhpps");
        ASSERT_FALSE(unchanged.isError()) << unchanged.toString().toStdString();
        EXPECT_TRUE(unchanged.toBool());
    }
}

TEST(ScientificGuiScriptFacade, ExplicitAllowlistRetainsLegacyCallsAndSharedApi)
{
    ScriptHost host;
    const QJSValue setup = host.evaluate(R"JS(
        var guiWindowMock = {
          callCount: 0,
          Run: function() { ++this.callCount; return 71; },
          SetValue: function() { ++this.callCount; return -1; },
          SetRaysNumber: function() { ++this.callCount; return -2; },
          Run: function() { ++this.callCount; return -5; },
          SetExportPhotonMapType: function() { ++this.callCount; return -3; },
          SetPhotonBufferSize: function() { ++this.callCount; return -4; },
          RunFluxAnalysis: function() { ++this.callCount; return -6; },
          Screenshot: function() { return 73; },
          windowTitle: function() { return 'LEAK'; },
          metaObject: function() { return 'LEAK'; }
        };
        this.tn = guiWindowMock;
        this.__tonatiuhppSceneApi = {
          getScene: function() { return 81; }, Clear: function() { return 82; },
          InsertScene: function() { return 83; }, saveScene: function() { return 84; },
          loadScene: function() { return 85; },
          FindInterception: function() { return 86; },
          ChangeSunPosition: function() { return 87; },
          SetValue: function() { return 72; },
          SetRaysGrid: function() { return 75; },
          SetRaysNumber: function() { return 76; },
          Run: function() { return 79; },
          RunFluxAnalysis: function() { return 80; },
          SetExportPhotonMapType: function() { return 77; },
          SetPhotonBufferSize: function() { return 78; }
        };
        this.__tonatiuhppTraceApi = {
          traceScene: function() { return 91; }, startTrace: function() { return 92; },
          cancelTrace: function() { return 93; }, finishTrace: function() { return 94; }
        };
    )JS", "<gui-facade-mock>");
    ASSERT_FALSE(setup.isError()) << setup.toString().toStdString();
    const QJSValue installed = host.evaluate(guiScriptFacadeBootstrap(), "<gui-facade-bootstrap>");
    ASSERT_FALSE(installed.isError()) << installed.toString().toStdString();
    const QJSValue allowed = host.evaluate(
        "tn === tonatiuh && tn.getScene() === 81 && "
        "tn.ChangeSunPosition(180, 60) === 87 && tn.traceScene({}) === 91 && "
        "tn.Run() === 79 && tn.SetValue('x','y','z') === 72 && "
        "tn.RunFluxAnalysis('x','front',1,2,2,'x.tsv',false) === 80 && "
        "tn.SetRaysGrid(64,48) === 75 && tn.SetRaysNumber(128) === 76 && "
        "tn.SetExportPhotonMapType('File') === 77 && "
        "tn.SetPhotonBufferSize(4096) === 78 && "
        "tn.Screenshot('x') === 73 && "
        "guiWindowMock.callCount === 0 && "
        "!('windowTitle' in tn) && !('metaObject' in tn);",
        "gui-allowlist.tnhpps");
    ASSERT_FALSE(allowed.isError()) << allowed.toString().toStdString();
    EXPECT_TRUE(allowed.toBool());
}

TEST(ScientificGuiScriptFacade, RejectsArbitraryQObjectMethodsAndKeepsScratchLocal)
{
    ScriptHost host;
    const QJSValue setup = host.evaluate(R"JS(
        var guiWindowMock = { windowTitle: function() { return 'LEAK'; },
                              deleteLater: function() { return 'LEAK'; } };
        this.tn = guiWindowMock;
        this.__tonatiuhppSceneApi = { getScene: function() {} };
        this.__tonatiuhppTraceApi = {};
    )JS", "<gui-facade-mock>");
    ASSERT_FALSE(setup.isError());
    const QJSValue installed = host.evaluate(guiScriptFacadeBootstrap(), "<gui-facade-bootstrap>");
    ASSERT_FALSE(installed.isError()) << installed.toString().toStdString();
    for (const QString& expression : {
             QStringLiteral("tn.windowTitle();"),
             QStringLiteral("tn.deleteLater();"),
             QStringLiteral("tn.runBenchmark('test.json');")
         }) {
        const QJSValue invalid = host.evaluate(expression, "blocked-qobject-method.tnhpps");
        ASSERT_TRUE(invalid.isError()) << expression.toStdString();
        EXPECT_TRUE(invalid.toString().contains("GUI script API does not support tn."));
    }
    const QJSValue scratch = host.evaluate(
        "tn.note = 'private'; tn.note === 'private' && "
        "guiWindowMock.note === undefined;", "gui-scratch.tnhpps");
    ASSERT_FALSE(scratch.isError()) << scratch.toString().toStdString();
    EXPECT_TRUE(scratch.toBool());
    const QJSValue overwrite = host.evaluate("tn.Run = function() {};", "gui-reserved.tnhpps");
    ASSERT_TRUE(overwrite.isError());
    EXPECT_TRUE(overwrite.toString().contains("Cannot replace"));
}

TEST(ScientificScriptFieldEdit, ResolvesLayoutInstanceAndEditsTransform)
{
    TonatiuhCore::initializeCoin();
    LegacySceneFixture fixture;
    const QJSValue result = fixture.host.evaluate(R"JS(
        var root = new NodeObject;
        root.setName('M3_Edit');
        var receiver = root.createNode('Receiver');
        tn.Clear();
        tn.InsertScene(root);
        tn.SetValue('//Node/M3_Edit/Receiver', 'translation', '0 0 1');
        receiver.getParameter('translation').trim() === '0 0 1';
    )JS", "field-edit.tnhpps");
    ASSERT_FALSE(result.isError()) << result.toString().toStdString();
    EXPECT_TRUE(result.toBool());
}

TEST(ScientificScriptFieldEdit, BadUrlsAndFieldsFailAtUserLine)
{
    TonatiuhCore::initializeCoin();
    LegacySceneFixture fixture;
    ASSERT_FALSE(fixture.host.evaluate(
        "var root = new NodeObject; root.setName('M3_Edit');"
        "var receiver = root.createNode('Receiver');"
        "tn.Clear(); tn.InsertScene(root);", "field-setup.tnhpps").isError());
    for (const QString& expr : {
             QStringLiteral("tn.SetValue('//Node/missing', 'translation', '0 0 9');"),
             QStringLiteral("tn.SetValue('//Node/M3_Edit/Receiver', 'bad-field', '1');"),
             QStringLiteral("tn.SetValue('', 'translation', '0 0 9');")
         }) {
        const QJSValue result = fixture.host.evaluate(expr, "field-negative.tnhpps");
        ASSERT_TRUE(result.isError()) << expr.toStdString();
        EXPECT_EQ(result.property("lineNumber").toInt(), 1);
        EXPECT_TRUE(result.toString().contains("tn.SetValue"));
    }
    const QJSValue preserved = fixture.host.evaluate(
        "receiver.getParameter('translation').trim() === '0 0 0';",
        "field-preserved.tnhpps");
    ASSERT_FALSE(preserved.isError());
    EXPECT_TRUE(preserved.toBool());
}

TEST(ScientificScriptFieldEdit, HostWriterCanRejectWithoutMutation)
{
    TonatiuhCore::initializeCoin();
    LegacySceneFixture fixture;
    ASSERT_FALSE(fixture.host.evaluate(
        "var root = new NodeObject; root.setName('M3_Edit');"
        "var receiver = root.createNode('Receiver');"
        "tn.Clear(); tn.InsertScene(root);", "field-setup.tnhpps").isError());
    auto* services = qobject_cast<ScriptSceneServices*>(fixture.host.engine()
        ->globalObject().property("__tonatiuhppSceneApi").toQObject());
    ASSERT_NE(services, nullptr);
    int calls = 0;
    services->setFieldWriter([&calls](SoNode* target, const QString& name,
                                       const QString& value, QString* error) {
        ++calls;
        EXPECT_NE(target, nullptr);
        EXPECT_EQ(name, "translation");
        EXPECT_EQ(value, "0 0 1");
        if (error) *error = "GUI edit rejected";
        return false;
    });
    const QJSValue error = fixture.host.evaluate(
        "tn.SetValue('//Node/M3_Edit/Receiver', 'translation', '0 0 1');",
        "field-callback-rejected.tnhpps");
    ASSERT_TRUE(error.isError());
    EXPECT_EQ(error.property("lineNumber").toInt(), 1);
    EXPECT_TRUE(error.toString().contains("GUI edit rejected"));
    EXPECT_EQ(calls, 1);
    const QJSValue preserved = fixture.host.evaluate(
        "receiver.getParameter('translation').trim() === '0 0 0';",
        "field-preserved.tnhpps");
    ASSERT_FALSE(preserved.isError());
    EXPECT_TRUE(preserved.toBool());
}

TEST(ScientificScriptGrid, ValidatesDimensionsBeforeHostCallback)
{
    TonatiuhCore::initializeCoin();
    LegacySceneFixture fixture;
    auto* services = qobject_cast<ScriptSceneServices*>(fixture.host.engine()
        ->globalObject().property("__tonatiuhppSceneApi").toQObject());
    ASSERT_NE(services, nullptr);
    int calls = 0;
    services->setSunGridWriter([&calls](int columns, int rows, QString*) {
        ++calls;
        EXPECT_EQ(columns, 64);
        EXPECT_EQ(rows, 48);
        return true;
    });
    for (const QString& expr : {
             QStringLiteral("tn.SetRaysGrid(0, 48);"),
             QStringLiteral("tn.SetRaysGrid(64, -1);"),
             QStringLiteral("tn.SetRaysGrid(1.5, 48);"),
             QStringLiteral("tn.SetRaysGrid(Infinity, 48);")
         }) {
        const QJSValue error = fixture.host.evaluate(expr, "grid-invalid.tnhpps");
        ASSERT_TRUE(error.isError());
        EXPECT_EQ(error.property("lineNumber").toInt(), 1);
        EXPECT_TRUE(error.toString().contains("tn.SetRaysGrid"));
    }
    EXPECT_EQ(calls, 0);
    const QJSValue accepted = fixture.host.evaluate("tn.SetRaysGrid(64, 48);",
                                                     "grid-valid.tnhpps");
    EXPECT_FALSE(accepted.isError()) << accepted.toString().toStdString();
    EXPECT_EQ(calls, 1);
}

TEST(ScientificScriptGrid, HostRejectionLeavesSubsequentCallsUsable)
{
    TonatiuhCore::initializeCoin();
    LegacySceneFixture fixture;
    auto* services = qobject_cast<ScriptSceneServices*>(fixture.host.engine()
        ->globalObject().property("__tonatiuhppSceneApi").toQObject());
    ASSERT_NE(services, nullptr);
    int calls = 0;
    services->setSunGridWriter([&calls](int, int, QString* error) {
        ++calls;
        if (calls == 1) {
            if (error) *error = "GUI grid rejected";
            return false;
        }
        return true;
    });
    const QJSValue first = fixture.host.evaluate("tn.SetRaysGrid(64, 48);",
                                                  "grid-rejected.tnhpps");
    ASSERT_TRUE(first.isError());
    EXPECT_TRUE(first.toString().contains("GUI grid rejected"));
    const QJSValue next = fixture.host.evaluate("tn.SetRaysGrid(64, 48);",
                                                 "grid-recovered.tnhpps");
    EXPECT_FALSE(next.isError()) << next.toString().toStdString();
    EXPECT_EQ(calls, 2);
}

TEST(ScientificScriptRunInputs, RunRayCountIsPerHostAndDoesNotTrace)
{
    TonatiuhCore::initializeCoin();
    LegacySceneFixture first;
    LegacySceneFixture second;
    auto* one = qobject_cast<ScriptSceneServices*>(first.host.engine()
        ->globalObject().property("__tonatiuhppSceneApi").toQObject());
    auto* two = qobject_cast<ScriptSceneServices*>(second.host.engine()
        ->globalObject().property("__tonatiuhppSceneApi").toQObject());
    ASSERT_NE(one, nullptr);
    ASSERT_NE(two, nullptr);
    EXPECT_FALSE(one->scriptRunRayCount().has_value());
    EXPECT_FALSE(two->scriptRunRayCount().has_value());
    const QJSValue value = first.host.evaluate(
        "tn.SetRaysNumber(128);", "run-ray-input.tnhpps");
    ASSERT_FALSE(value.isError()) << value.toString().toStdString();
    EXPECT_TRUE(value.isUndefined()); // Legacy setter returned void.
    ASSERT_TRUE(one->scriptRunRayCount().has_value());
    EXPECT_EQ(*one->scriptRunRayCount(), 128u);
    EXPECT_FALSE(two->scriptRunRayCount().has_value());
    // This method changes only script input state. There is no implicit run.
}

TEST(ScientificScriptRunInputs, InvalidCountsFailAtScriptLineAndPreserveState)
{
    TonatiuhCore::initializeCoin();
    LegacySceneFixture fixture;
    auto* scene = qobject_cast<ScriptSceneServices*>(fixture.host.engine()
        ->globalObject().property("__tonatiuhppSceneApi").toQObject());
    ASSERT_NE(scene, nullptr);
    ASSERT_FALSE(fixture.host.evaluate("tn.SetRaysNumber(64);", "run-setup.tnhpps").isError());
    int callbacks = 0;
    scene->setRunRayCountWriter([&callbacks](uint, QString*) {
        ++callbacks;
        return true;
    });
    const QStringList failures = {
        "tn.SetRaysNumber(0);", "tn.SetRaysNumber(-1);",
        "tn.SetRaysNumber(1.5);", "tn.SetRaysNumber(Infinity);",
        "tn.SetRaysNumber(NaN);", "tn.SetRaysNumber('128');",
        "tn.SetRaysNumber(true);", "tn.SetRaysNumber(4294967296);",
        "tn.SetRaysNumber();"
    };
    for (const QString& expr : failures) {
        const QJSValue rejected = fixture.host.evaluate(
            "var sentinel = 1;\n" + expr, "run-rays-invalid.tnhpps");
        ASSERT_TRUE(rejected.isError()) << expr.toStdString();
        EXPECT_EQ(rejected.property("lineNumber").toInt(), 2) << expr.toStdString();
        EXPECT_TRUE(rejected.toString().contains("tn.SetRaysNumber"));
        ASSERT_TRUE(scene->scriptRunRayCount().has_value());
        EXPECT_EQ(*scene->scriptRunRayCount(), 64u);
    }
    EXPECT_EQ(callbacks, 0);
}

TEST(ScientificScriptRunInputs, HostRejectionIsAtomicAndRecoveryWorks)
{
    TonatiuhCore::initializeCoin();
    LegacySceneFixture fixture;
    auto* scene = qobject_cast<ScriptSceneServices*>(fixture.host.engine()
        ->globalObject().property("__tonatiuhppSceneApi").toQObject());
    ASSERT_NE(scene, nullptr);
    ASSERT_FALSE(fixture.host.evaluate("tn.SetRaysNumber(64);", "run-setup.tnhpps").isError());
    int callbacks = 0;
    scene->setRunRayCountWriter([&callbacks](uint rays, QString* error) {
        ++callbacks;
        EXPECT_EQ(rays, 128u);
        if (callbacks == 1) {
            if (error) *error = "GUI refused ray-count setting";
            return false;
        }
        return true;
    });
    const QJSValue rejected = fixture.host.evaluate(
        "tn.SetRaysNumber(128);", "run-rejected.tnhpps");
    ASSERT_TRUE(rejected.isError());
    EXPECT_TRUE(rejected.toString().contains("GUI refused ray-count setting"));
    ASSERT_TRUE(scene->scriptRunRayCount().has_value());
    EXPECT_EQ(*scene->scriptRunRayCount(), 64u);
    const QJSValue recovered = fixture.host.evaluate(
        "tn.SetRaysNumber(128);", "run-recovered.tnhpps");
    EXPECT_FALSE(recovered.isError()) << recovered.toString().toStdString();
    ASSERT_TRUE(scene->scriptRunRayCount().has_value());
    EXPECT_EQ(*scene->scriptRunRayCount(), 128u);
    EXPECT_EQ(callbacks, 2);
}

TEST(ScientificScriptExportConfig, BeforeSelectingExporterRetainsLegacyNoOpPolicy)
{
    TonatiuhCore::initializeCoin();
    LegacySceneFixture fixture;
    auto* scene = qobject_cast<ScriptSceneServices*>(fixture.host.engine()
        ->globalObject().property("__tonatiuhppSceneApi").toQObject());
    ASSERT_NE(scene, nullptr);
    int calls = 0;
    scene->setPhotonExportWriter([&](const ScriptPhotonExportChange&, QString*) {
        ++calls; return true;
    });
    const auto result = fixture.host.evaluate(
        "tn.SetExportCoordinates(false, false); tn.AddExportSurfaceURL('//Node/X');",
        "export-noop.tnhpps");
    EXPECT_FALSE(result.isError()) << result.toString().toStdString();
    EXPECT_EQ(calls, 0);
    EXPECT_FALSE(scene->scriptPhotonExportState().settingsPresent);
    EXPECT_FALSE(scene->scriptPhotonExportState().saveCoordinates.has_value());
    EXPECT_TRUE(scene->scriptPhotonExportState().surfaces.isEmpty());
}

TEST(ScientificScriptExportConfig, OptionsArePerHostAndDoNotRunOrExport)
{
    TonatiuhCore::initializeCoin();
    LegacySceneFixture one;
    LegacySceneFixture two;
    auto* first = qobject_cast<ScriptSceneServices*>(one.host.engine()
        ->globalObject().property("__tonatiuhppSceneApi").toQObject());
    auto* second = qobject_cast<ScriptSceneServices*>(two.host.engine()
        ->globalObject().property("__tonatiuhppSceneApi").toQObject());
    ASSERT_NE(first, nullptr);
    ASSERT_NE(second, nullptr);
    const auto outcome = one.host.evaluate(R"JS(
      tn.SetExportPhotonMapType('File');
      tn.SetExportCoordinates(true, false);
      tn.SetExportIntersectionSurface(true);
      tn.SetExportIntersectionSurfaceSide(false);
      tn.SetExportPreviousNextPhotonID(true);
      tn.SetExportTypeParameterValue('ExportFile','configured-only');
      tn.AddExportSurfaceURL('//Node/X/Receiver');
      tn.SetPhotonBufferSize(8192);
      tn.SetPhotonBufferAppend(true);
      tn.SetExportAllPhotonMap();
      tn.AddExportSurfaceURL('//Node/Y/Receiver');
    )JS", "export-options.tnhpps");
    EXPECT_FALSE(outcome.isError()) << outcome.toString().toStdString();
    const auto& state = first->scriptPhotonExportState();
    EXPECT_TRUE(state.settingsPresent);
    ASSERT_TRUE(state.exporterName.has_value());
    EXPECT_EQ(*state.exporterName, "File");
    ASSERT_TRUE(state.saveCoordinates.has_value());
    EXPECT_EQ(*state.saveCoordinates, true);
    ASSERT_TRUE(state.saveCoordinatesGlobal.has_value());
    EXPECT_EQ(*state.saveCoordinatesGlobal, false);
    ASSERT_TRUE(state.saveSurfaceID.has_value());
    EXPECT_EQ(*state.saveSurfaceID, true);
    ASSERT_TRUE(state.saveSurfaceSide.has_value());
    EXPECT_EQ(*state.saveSurfaceSide, false);
    ASSERT_TRUE(state.savePhotonsID.has_value());
    EXPECT_EQ(*state.savePhotonsID, true);
    EXPECT_EQ(state.parameters.value("ExportFile"), "configured-only");
    EXPECT_EQ(state.surfaces, QStringList({"//Node/Y/Receiver"}));
    ASSERT_TRUE(state.bufferSize.has_value());
    EXPECT_EQ(*state.bufferSize, 8192u);
    ASSERT_TRUE(state.bufferAppend.has_value());
    EXPECT_EQ(*state.bufferAppend, true);
    EXPECT_FALSE(second->scriptPhotonExportState().settingsPresent);
    EXPECT_TRUE(second->scriptPhotonExportState().surfaces.isEmpty());
    // Only state validation: no Run, worker, exporter or filesystem I/O.
}

TEST(ScientificScriptExportConfig, RejectsBadArgumentsAtCallSiteWithoutStateMutation)
{
    TonatiuhCore::initializeCoin();
    LegacySceneFixture fixture;
    auto* scene = qobject_cast<ScriptSceneServices*>(fixture.host.engine()
        ->globalObject().property("__tonatiuhppSceneApi").toQObject());
    ASSERT_NE(scene, nullptr);
    const QStringList invalid = {
        "tn.SetExportPhotonMapType();",
        "tn.SetExportPhotonMapType('');",
        "tn.SetExportCoordinates(true);",
        "tn.SetExportCoordinates(1, false);",
        "tn.SetExportIntersectionSurface();",
        "tn.SetExportIntersectionSurfaceSide('yes');",
        "tn.SetExportPreviousNextPhotonID(1);",
        "tn.SetExportTypeParameterValue('', 'value');",
        "tn.SetExportTypeParameterValue('name');",
        "tn.AddExportSurfaceURL('');",
        "tn.SetPhotonBufferSize(0);",
        "tn.SetPhotonBufferSize(1.5);",
        "tn.SetPhotonBufferSize(4294967296);",
        "tn.SetPhotonBufferAppend();"
    };
    for (const QString& expr : invalid) {
        const auto error = fixture.host.evaluate(
            "var guard = 42;\n" + expr, "export-bad-input.tnhpps");
        ASSERT_TRUE(error.isError()) << expr.toStdString();
        EXPECT_EQ(error.property("lineNumber").toInt(), 2) << expr.toStdString();
        EXPECT_TRUE(error.toString().contains("tn.")) << error.toString().toStdString();
    }
    EXPECT_FALSE(scene->scriptPhotonExportState().settingsPresent);
    EXPECT_FALSE(scene->scriptPhotonExportState().bufferSize.has_value());
}

TEST(ScientificScriptExportConfig, HostFailureIsAtomicAndRecoveryWorks)
{
    TonatiuhCore::initializeCoin();
    LegacySceneFixture fixture;
    auto* scene = qobject_cast<ScriptSceneServices*>(fixture.host.engine()
        ->globalObject().property("__tonatiuhppSceneApi").toQObject());
    ASSERT_NE(scene, nullptr);
    ASSERT_FALSE(fixture.host.evaluate("tn.SetExportPhotonMapType('File');").isError());
    int calls = 0;
    scene->setPhotonExportWriter([&](const ScriptPhotonExportChange& change, QString* error) {
        if (change.kind != ScriptPhotonExportChange::Kind::Coordinates) return true;
        ++calls;
        if (calls == 1) {
            if (error) *error = "GUI retained failed photon export buffer";
            return false;
        }
        return true;
    });
    const auto rejected = fixture.host.evaluate(
        "tn.SetExportCoordinates(false, false);", "export-host-failure.tnhpps");
    EXPECT_TRUE(rejected.isError());
    EXPECT_TRUE(rejected.toString().contains("GUI retained failed photon export buffer"));
    EXPECT_FALSE(scene->scriptPhotonExportState().saveCoordinates.has_value());
    const auto recovered = fixture.host.evaluate(
        "tn.SetExportCoordinates(false, false);", "export-recovery.tnhpps");
    EXPECT_FALSE(recovered.isError()) << recovered.toString().toStdString();
    ASSERT_TRUE(scene->scriptPhotonExportState().saveCoordinates.has_value());
    EXPECT_EQ(*scene->scriptPhotonExportState().saveCoordinates, false);
    EXPECT_EQ(calls, 2);
}

TEST(ScientificScriptExportConfig, BufferSettingsIndependentOfExporterChoice)
{
    TonatiuhCore::initializeCoin();
    LegacySceneFixture fixture;
    auto* scene = qobject_cast<ScriptSceneServices*>(fixture.host.engine()
        ->globalObject().property("__tonatiuhppSceneApi").toQObject());
    ASSERT_NE(scene, nullptr);
    const auto result = fixture.host.evaluate(
        "tn.SetPhotonBufferSize(4096);tn.SetPhotonBufferAppend(false);",
        "buffer-settings.tnhpps");
    EXPECT_FALSE(result.isError()) << result.toString().toStdString();
    ASSERT_TRUE(scene->scriptPhotonExportState().bufferSize.has_value());
    EXPECT_EQ(*scene->scriptPhotonExportState().bufferSize, 4096u);
    ASSERT_TRUE(scene->scriptPhotonExportState().bufferAppend.has_value());
    EXPECT_EQ(*scene->scriptPhotonExportState().bufferAppend, false);
    EXPECT_FALSE(scene->scriptPhotonExportState().settingsPresent);
}

// M3 Phase 3B3A: validate only the nonvisual script request and JS errors.
// No filesystem, optical calculation or GUI event loop is required here.
// Phase 3B3B0: protect actual GUI flux TSV bytes and file-error signaling.
// These widget-free tests do not claim cross-host numerical flux parity.
// M3 Phase 3B3B1A: real Coin shape/profile inputs with synthetic photons.
// All grid numerics are shared with the GUI; ray propagation is unchanged.
// M3 integrated candidate: one script-facing optional seed in both JS hosts.
// The existing seven-argument signature must remain valid and unchanged.
TEST(ScientificFluxAnalysisOutput, AtomicReplacementRetainsPriorBytesOnInvalidInput)
{
    QTemporaryDir temp;
    ASSERT_TRUE(temp.isValid());
    const QString destination = temp.filePath("existing.txt");
    QFile initial(destination);
    ASSERT_TRUE(initial.open(QIODevice::WriteOnly));
    ASSERT_EQ(initial.write("old-artifact"), 12ll);
    initial.close();

    Matrix2D<int> bins(2, 2);
    bins.fill(1);
    const Box2D box(vec2d(0., 0.), vec2d(2., 2.));
    QString error;
    ASSERT_TRUE(FluxAnalysisTsvWriter::write(destination, bins, box, 1., false, &error))
        << error.toStdString();
    QFile before(destination);
    ASSERT_TRUE(before.open(QIODevice::ReadOnly));
    const QByteArray accepted = before.readAll();
    EXPECT_EQ(accepted, QByteArray("1\t1\t\n1\t1\t\n"));
    before.close();

    Matrix2D<int> empty;
    EXPECT_FALSE(FluxAnalysisTsvWriter::write(destination, empty, box, 1., false, &error));
    EXPECT_TRUE(error.contains("invalid cells"));
    QFile after(destination);
    ASSERT_TRUE(after.open(QIODevice::ReadOnly));
    EXPECT_EQ(after.readAll(), accepted);
}

TEST(ScientificSharedFluxScript, ExplicitSeedAndGridReachOneDomainExecutor)
{
    TonatiuhCore::initializeCoin();
    LegacySceneFixture fixture;
    ASSERT_FALSE(fixture.host.evaluate("tn.Clear();tn.SetRaysGrid(64,48);").isError());
    auto* services = qobject_cast<ScriptSceneServices*>(fixture.host.engine()
        ->globalObject().property("__tonatiuhppSceneApi").toQObject());
    ASSERT_NE(services, nullptr);
    int calls = 0;
    services->setFluxAnalysisExecutor([&](const ScriptFluxAnalysisRequest& request, QString*) {
        ++calls;
        EXPECT_EQ(request.sunWidthDivisions, 64);
        EXPECT_EQ(request.sunHeightDivisions, 48);
        EXPECT_EQ(request.rays, 4096u);
        if (calls == 1) {
            EXPECT_FALSE(request.replaceExistingOutput);
            EXPECT_TRUE(request.masterSeed.has_value());
            if (request.masterSeed) EXPECT_EQ(*request.masterSeed, 123456789u);
        } else {
            EXPECT_TRUE(request.replaceExistingOutput);
            EXPECT_FALSE(request.masterSeed.has_value());
        }
        return true;
    });
    const auto seeded = fixture.host.evaluate(
        "tn.RunFluxAnalysis('//Node/Receiver','front',4096,8,8,'grid.txt',false,123456789);",
        "shared-flux-seeded.tnhpps");
    EXPECT_FALSE(seeded.isError()) << seeded.toString().toStdString();
    EXPECT_TRUE(seeded.isUndefined());
    const auto legacy = fixture.host.evaluate(
        "tn.RunFluxAnalysis('//Node/Receiver','front',4096,8,8,'grid.txt',false);",
        "shared-flux-seven-args.tnhpps");
    EXPECT_FALSE(legacy.isError()) << legacy.toString().toStdString();
    EXPECT_TRUE(legacy.isUndefined());
    EXPECT_EQ(calls, 2);
}

TEST(ScientificSharedFluxScript, RejectsBadSeedsBeforeInvokingExecutor)
{
    TonatiuhCore::initializeCoin();
    LegacySceneFixture fixture;
    ASSERT_FALSE(fixture.host.evaluate("tn.Clear();").isError());
    auto* services = qobject_cast<ScriptSceneServices*>(fixture.host.engine()
        ->globalObject().property("__tonatiuhppSceneApi").toQObject());
    ASSERT_NE(services, nullptr);
    int calls = 0;
    services->setFluxAnalysisExecutor([&](const ScriptFluxAnalysisRequest&, QString*) {
        ++calls;
        return true;
    });
    for (const QString& value : {QStringLiteral("-1"), QStringLiteral("NaN"),
                                 QStringLiteral("1.5"), QStringLiteral("'123'"),
                                 QStringLiteral("4294967296")}) {
        const auto bad = fixture.host.evaluate(
            "tn.RunFluxAnalysis('//Node/Receiver','front',128,8,8,'grid.txt',false," +
            value + ");", "shared-flux-invalid-seed.tnhpps");
        EXPECT_TRUE(bad.isError()) << value.toStdString();
        EXPECT_TRUE(bad.toString().contains("seed")) << bad.toString().toStdString();
    }
    EXPECT_EQ(calls, 0);
    const auto good = fixture.host.evaluate(
        "tn.RunFluxAnalysis('//Node/Receiver','front',128,8,8,'grid.txt',false,0);",
        "shared-flux-recovered.tnhpps");
    EXPECT_FALSE(good.isError()) << good.toString().toStdString();
    EXPECT_EQ(calls, 1);
}

TEST(ScientificSharedFluxBackend, BoundedSeededTraceProducesGridAndProtectsOutput)
{
    TonatiuhCore::initializeCoin();
    LegacySceneFixture fixture;
    const auto scene = fixture.host.evaluate(R"JS(
        var root = new NodeObject;
        root.setName('SharedFluxBackend');
        var receiver = root.createNode('Receiver');
        var shape = receiver.createShape();
        shape.setName('Shape');
        shape.insertSurface('Planar');
        var profile = shape.insertProfile('Box');
        profile.setParameter('uSize', '8');
        profile.setParameter('vSize', '8');
        shape.insertMaterial('Absorber');
        tn.Clear(); tn.InsertScene(root);
        var sun = tn.getScene().getPart('world.sun.position');
        sun.setParameter('azimuth', '180');
        sun.setParameter('elevation', '90');
    )JS", "shared-flux-real-scene.tnhpps");
    ASSERT_FALSE(scene.isError()) << scene.toString().toStdString();
    QTemporaryDir temp;
    ASSERT_TRUE(temp.isValid());
    ScriptFluxAnalysisRequest request;
    request.surfaceURL = "//Node/SharedFluxBackend/Receiver/Shape";
    request.surfaceSide = "front";
    request.rays = 256;
    request.heightDivisions = 2;
    request.widthDivisions = 2;
    request.sunWidthDivisions = 64;
    request.sunHeightDivisions = 48;
    request.masterSeed = 0;
    request.fileName = temp.filePath("flux.txt");
    QString error;
    ASSERT_TRUE(HeadlessScriptFluxBackend::run(fixture.document.scene(), request, &error))
        << error.toStdString();
    QFile artifact(request.fileName);
    ASSERT_TRUE(artifact.open(QIODevice::ReadOnly));
    const QByteArray contents = artifact.readAll();
    EXPECT_EQ(contents.count('\n'), 2);
    EXPECT_TRUE(contents.contains('\t'));
    artifact.close();
    EXPECT_FALSE(HeadlessScriptFluxBackend::run(fixture.document.scene(), request, &error));
    EXPECT_TRUE(error.contains("already exists")) << error.toStdString();
    QFile unchanged(request.fileName);
    ASSERT_TRUE(unchanged.open(QIODevice::ReadOnly));
    EXPECT_EQ(unchanged.readAll(), contents);
    unchanged.close();
    // Explicitly enabled legacy overwrite still uses the same nonvisual
    // backend and must be byte-repeatable under a matched effective seed.
    request.replaceExistingOutput = true;
    ASSERT_TRUE(HeadlessScriptFluxBackend::run(fixture.document.scene(), request, &error))
        << error.toStdString();
    QFile replaced(request.fileName);
    ASSERT_TRUE(replaced.open(QIODevice::ReadOnly));
    EXPECT_EQ(replaced.readAll(), contents);
    replaced.close();
    request.surfaceURL = "//Node/SharedFluxBackend/Receiver/Missing";
    request.fileName = temp.filePath("invalid.txt");
    EXPECT_FALSE(HeadlessScriptFluxBackend::run(fixture.document.scene(), request, &error));
    EXPECT_TRUE(error.contains("selected surface")) << error.toStdString();
    EXPECT_FALSE(QFile::exists(request.fileName));
}

TEST(ScientificFluxGridComputation, PlanarSelectedSideCountsAndNormalizes)
{
    TonatiuhCore::initializeCoin();
    LegacySceneFixture fixture;
    const QJSValue setup = fixture.host.evaluate(R"JS(
        var root = new NodeObject;
        root.setName('GridFixture');
        var receiver = root.createNode('Receiver');
        var shape = receiver.createShape();
        shape.setName('Shape');
        shape.insertSurface('Planar');
        var profile = shape.insertProfile('Box');
        profile.setParameter('uSize', '8');
        profile.setParameter('vSize', '8');
        shape.insertMaterial('Absorber');
        tn.Clear();
        tn.InsertScene(root);
    )JS", "flux-grid-fixture.tnhpps");
    ASSERT_FALSE(setup.isError()) << setup.toString().toStdString();
    SceneInstanceTree tree = SceneInstanceBuilder::build(fixture.document.scene());
    ASSERT_NE(tree.layoutRoot, nullptr);
    tree.layoutRoot->updateTree(Transform::Identity);
    InstanceNode* target = nullptr;
    std::function<void(InstanceNode*)> findShape = [&](InstanceNode* node) {
        if (!node) return;
        if (dynamic_cast<TShapeKit*>(node->getNode())) target = node;
        for (InstanceNode* child : node->children) findShape(child);
    };
    findShape(tree.layoutRoot);
    ASSERT_NE(target, nullptr);

    const std::vector<Photon> photons{
        Photon(1, vec3d(0., 0., 0.), target, true),
        Photon(2, vec3d(0., 0., 0.), target, true),
        Photon(3, vec3d(0., 0., 0.), target, false)
    };
    FluxGridComputationResult front;
    QString error;
    ASSERT_TRUE(FluxGridComputation::calculate(photons, target, "front", 4, 4,
                                                2., &front, &error)) << error.toStdString();
    int frontCount = 0;
    for (int n : front.binsPhotons.data()) frontCount += n;
    EXPECT_EQ(frontCount, 2);
    EXPECT_DOUBLE_EQ(front.powerTotal, 4.);
    EXPECT_EQ(front.photonsMax, 2);
    EXPECT_EQ(front.binsFlux.rows(), 4);
    EXPECT_EQ(front.binsFlux.cols(), 4);
    for (double flux : front.binsFlux.data())
        EXPECT_TRUE(std::isfinite(flux));

    FluxGridComputationResult back;
    ASSERT_TRUE(FluxGridComputation::calculate(photons, target, "back", 4, 4,
                                                2., &back, &error)) << error.toStdString();
    int backCount = 0;
    for (int n : back.binsPhotons.data()) backCount += n;
    EXPECT_EQ(backCount, 1);
    EXPECT_DOUBLE_EQ(back.powerTotal, 2.);
    EXPECT_EQ(back.photonsMax, 1);
}

TEST(ScientificFluxGridComputation, InvalidRequestsFailBeforeAnyOutputMutation)
{
    FluxGridComputationResult output;
    output.powerTotal = 42.;
    QString error;
    const std::vector<Photon> empty;
    EXPECT_FALSE(FluxGridComputation::calculate(empty, nullptr, "front", 4, 4,
                                                1., &output, &error));
    EXPECT_TRUE(error.contains("selected surface")) << error.toStdString();
    EXPECT_DOUBLE_EQ(output.powerTotal, 42.);
}

TEST(ScientificFluxAnalysisOutput, PreservesLegacyTsvLayoutWithAndWithoutCoordinates)
{
    QTemporaryDir temp;
    ASSERT_TRUE(temp.isValid());
    Matrix2D<int> bins(2, 2);
    bins(0, 0) = 1;
    bins(0, 1) = 2;
    bins(1, 0) = 3;
    bins(1, 1) = 4;
    const Box2D box(vec2d(0., 0.), vec2d(2., 4.));
    QString error;
    const QString simple = temp.filePath("plain.txt");
    ASSERT_TRUE(FluxAnalysisTsvWriter::write(simple, bins, box, 0.25, false, &error))
        << error.toStdString();
    QFile plain(simple);
    ASSERT_TRUE(plain.open(QIODevice::ReadOnly));
    EXPECT_EQ(plain.readAll(), QByteArray("0.25\t0.5\t\n0.75\t1\t\n"));

    const QString coords = temp.filePath("coords.txt");
    ASSERT_TRUE(FluxAnalysisTsvWriter::write(coords, bins, box, 0.25, true, &error))
        << error.toStdString();
    QFile positioned(coords);
    ASSERT_TRUE(positioned.open(QIODevice::ReadOnly));
    EXPECT_EQ(positioned.readAll(), QByteArray(
        "x(m)\ty(m)\tFlux(W/m2)\n"
        "0.5\t1\t0.25\n"
        "0.5\t3\t0.5\n"
        "1.5\t1\t0.75\n"
        "1.5\t3\t1\n"));
}

TEST(ScientificFluxAnalysisOutput, MissingOutputDirectoryRejectsWithoutCreatingFile)
{
    QTemporaryDir temp;
    ASSERT_TRUE(temp.isValid());
    Matrix2D<int> bins(2, 2);
    bins.fill(1);
    QString error;
    const QString destination = temp.filePath("nonexistent/flux.txt");
    EXPECT_FALSE(FluxAnalysisTsvWriter::write(
        destination, bins, Box2D(vec2d(0., 0.), vec2d(2., 2.)),
        1., false, &error));
    EXPECT_TRUE(error.contains("Cannot open flux output")) << error.toStdString();
    EXPECT_FALSE(QFile::exists(destination));
}

TEST(ScientificFluxAnalysisOutput, EmptyGridRejectsBeforeTouchingDisk)
{
    QTemporaryDir temp;
    ASSERT_TRUE(temp.isValid());
    QString error;
    Matrix2D<int> empty;
    const QString destination = temp.filePath("flux.txt");
    EXPECT_FALSE(FluxAnalysisTsvWriter::write(
        destination, empty, Box2D(vec2d(0., 0.), vec2d(2., 2.)),
        1., true, &error));
    EXPECT_TRUE(error.contains("invalid cells")) << error.toStdString();
    EXPECT_FALSE(QFile::exists(destination));
}

TEST(ScientificScriptFluxAnalysisDispatch, PreservesSevenArgumentRequestAndUndefinedResult)
{
    TonatiuhCore::initializeCoin();
    LegacySceneFixture fixture;
    ASSERT_FALSE(fixture.host.evaluate("tn.Clear();").isError());
    auto* services = qobject_cast<ScriptSceneServices*>(fixture.host.engine()
        ->globalObject().property("__tonatiuhppSceneApi").toQObject());
    ASSERT_NE(services, nullptr);
    int dispatched = 0;
    services->setFluxAnalysisExecutor([&](const ScriptFluxAnalysisRequest& request,
                                           QString*) {
        ++dispatched;
        EXPECT_EQ(request.surfaceURL, "//Node/Receiver");
        EXPECT_EQ(request.surfaceSide, "back");
        EXPECT_EQ(request.rays, 2048u);
        EXPECT_EQ(request.heightDivisions, 8);
        EXPECT_EQ(request.widthDivisions, 12);
        EXPECT_EQ(request.fileName, "flux-output.tsv");
        EXPECT_TRUE(request.saveCoordinates);
        return true;
    });
    const QJSValue result = fixture.host.evaluate(
        "tn.RunFluxAnalysis('//Node/Receiver', 'back', 2048, 8, 12, "
        "'flux-output.tsv', true);", "flux-request.tnhpps");
    EXPECT_FALSE(result.isError()) << result.toString().toStdString();
    EXPECT_TRUE(result.isUndefined());
    EXPECT_EQ(dispatched, 1);
}

TEST(ScientificScriptFluxAnalysisDispatch, MissingBackendFailsAtUserScriptLine)
{
    TonatiuhCore::initializeCoin();
    LegacySceneFixture fixture;
    ASSERT_FALSE(fixture.host.evaluate("tn.Clear();").isError());
    const QJSValue result = fixture.host.evaluate(
        "var marker = 1;\ntn.RunFluxAnalysis('//Node/Receiver', 'front', 128, 8, 12, "
        "'flux.tsv', false);", "flux-no-backend.tnhpps");
    ASSERT_TRUE(result.isError());
    EXPECT_EQ(result.property("lineNumber").toInt(), 2);
    EXPECT_TRUE(result.toString().contains("tn.RunFluxAnalysis"));
    EXPECT_TRUE(result.toString().contains("no nonvisual flux-analysis backend"));
}

TEST(ScientificScriptFluxAnalysisDispatch, InvalidInputsCannotReachHostAdapter)
{
    TonatiuhCore::initializeCoin();
    LegacySceneFixture fixture;
    ASSERT_FALSE(fixture.host.evaluate("tn.Clear();").isError());
    auto* services = qobject_cast<ScriptSceneServices*>(fixture.host.engine()
        ->globalObject().property("__tonatiuhppSceneApi").toQObject());
    ASSERT_NE(services, nullptr);
    int dispatched = 0;
    services->setFluxAnalysisExecutor([&](const ScriptFluxAnalysisRequest&, QString*) {
        ++dispatched;
        return true;
    });
    const QStringList invalidCalls = {
        "tn.RunFluxAnalysis('', 'front', 128, 8, 12, 'flux.tsv', false);",
        "tn.RunFluxAnalysis('//Node/Receiver', 'side', 128, 8, 12, 'flux.tsv', false);",
        "tn.RunFluxAnalysis('//Node/Receiver', 'front', 0, 8, 12, 'flux.tsv', false);",
        "tn.RunFluxAnalysis('//Node/Receiver', 'front', NaN, 8, 12, 'flux.tsv', false);",
        "tn.RunFluxAnalysis('//Node/Receiver', 'front', 2.5, 8, 12, 'flux.tsv', false);",
        "tn.RunFluxAnalysis('//Node/Receiver', 'front', 128, 1, 12, 'flux.tsv', false);",
        "tn.RunFluxAnalysis('//Node/Receiver', 'front', 128, 8, -2, 'flux.tsv', false);",
        "tn.RunFluxAnalysis('//Node/Receiver', 'front', 128, 8, 12, '', false);",
        "tn.RunFluxAnalysis('//Node/Receiver', 'front', 128, 8, 12, 'flux.tsv', 1);",
    };
    for (const QString& call : invalidCalls) {
        const QJSValue result = fixture.host.evaluate(
            QStringLiteral("var marker = 1;\n") + call, "flux-invalid.tnhpps");
        ASSERT_TRUE(result.isError()) << call.toStdString();
        EXPECT_EQ(result.property("lineNumber").toInt(), 2);
        EXPECT_TRUE(result.toString().contains("tn.RunFluxAnalysis"));
    }
    EXPECT_EQ(dispatched, 0);
}

TEST(ScientificScriptFluxAnalysisDispatch, HostRejectionPropagatesAndLaterCallSucceeds)
{
    TonatiuhCore::initializeCoin();
    LegacySceneFixture fixture;
    ASSERT_FALSE(fixture.host.evaluate("tn.Clear();").isError());
    auto* services = qobject_cast<ScriptSceneServices*>(fixture.host.engine()
        ->globalObject().property("__tonatiuhppSceneApi").toQObject());
    ASSERT_NE(services, nullptr);
    int dispatched = 0;
    services->setFluxAnalysisExecutor([&](const ScriptFluxAnalysisRequest&, QString* error) {
        ++dispatched;
        if (dispatched == 1) {
            if (error) *error = "tn.RunFluxAnalysis: selected surface is unavailable";
            return false;
        }
        return true;
    });
    const char* call = "tn.RunFluxAnalysis('//Node/Receiver', 'front', 128, 8, 12, 'flux.tsv', false);";
    const QJSValue rejected = fixture.host.evaluate(
        QStringLiteral("var marker = 1;\n") + QString::fromUtf8(call),
        "flux-rejected.tnhpps");
    ASSERT_TRUE(rejected.isError());
    EXPECT_EQ(rejected.property("lineNumber").toInt(), 2);
    EXPECT_TRUE(rejected.toString().contains("selected surface is unavailable"));
    const QJSValue accepted = fixture.host.evaluate(
        QString::fromUtf8(call), "flux-recovery.tnhpps");
    EXPECT_FALSE(accepted.isError()) << accepted.toString().toStdString();
    EXPECT_EQ(dispatched, 2);
}

TEST(ScientificScriptRunDispatch, MissingBackendRejectsAtScriptLine)
{
    TonatiuhCore::initializeCoin();
    LegacySceneFixture fixture;
    ASSERT_FALSE(fixture.host.evaluate("tn.Clear();").isError());
    const auto result = fixture.host.evaluate(
        "var marker = 1;\ntn.Run();", "run-no-executor.tnhpps");
    ASSERT_TRUE(result.isError());
    EXPECT_EQ(result.property("lineNumber").toInt(), 2);
    EXPECT_TRUE(result.toString().contains("tn.Run"));
}

TEST(ScientificScriptRunDispatch, SnapshotsEffectiveRunInputsWithoutTracing)
{
    TonatiuhCore::initializeCoin();
    LegacySceneFixture fixture;
    ASSERT_FALSE(fixture.host.evaluate("tn.Clear();").isError());
    auto* scene = qobject_cast<ScriptSceneServices*>(fixture.host.engine()
        ->globalObject().property("__tonatiuhppSceneApi").toQObject());
    ASSERT_NE(scene, nullptr);
    int runs = 0;
    scene->setRunExecutor([&](const ScriptRunRequest& request, QString*) {
        ++runs;
        EXPECT_EQ(request.scene, fixture.document.scene());
        EXPECT_EQ(request.rays, 2048u);
        EXPECT_TRUE(request.explicitRays.has_value());
        EXPECT_EQ(request.sunWidthDivisions, 64);
        EXPECT_EQ(request.sunHeightDivisions, 48);
        EXPECT_TRUE(request.exportOptions.settingsPresent);
        EXPECT_EQ(request.exportOptions.exporterName.value_or(""), "File");
        EXPECT_EQ(request.exportOptions.parameters.value("ExportFile"), "PhotonMap");
        EXPECT_EQ(request.exportOptions.surfaces, QStringList({"//Node/Shape"}));
        EXPECT_EQ(request.exportOptions.bufferSize.value_or(0), 4096u);
        return true;
    });
    const auto result = fixture.host.evaluate(R"JS(
        tn.SetRaysNumber(2048);
        tn.SetRaysGrid(64,48);
        tn.SetExportPhotonMapType('File');
        tn.SetExportTypeParameterValue('ExportFile','PhotonMap');
        tn.SetPhotonBufferSize(4096);
        tn.AddExportSurfaceURL('//Node/Shape');
        tn.Run();
    )JS", "run-snapshot.tnhpps");
    EXPECT_FALSE(result.isError()) << result.toString().toStdString();
    EXPECT_TRUE(result.isUndefined());
    EXPECT_EQ(runs, 1);
}

TEST(ScientificScriptRunDispatch, RejectionIsAtomicAndSameHostRecovers)
{
    TonatiuhCore::initializeCoin();
    LegacySceneFixture fixture;
    ASSERT_FALSE(fixture.host.evaluate("tn.Clear();tn.SetRaysNumber(128);").isError());
    auto* scene = qobject_cast<ScriptSceneServices*>(fixture.host.engine()
        ->globalObject().property("__tonatiuhppSceneApi").toQObject());
    ASSERT_NE(scene, nullptr);
    int calls = 0;
    scene->setRunExecutor([&](const ScriptRunRequest& request, QString* error) {
        ++calls;
        EXPECT_EQ(request.rays, 128u);
        if (calls == 1) {
            if (error) *error = "tn.Run: output directory must be empty";
            return false;
        }
        return true;
    });
    const auto rejected = fixture.host.evaluate(
        "var marker = 1;\ntn.Run();", "run-rejected.tnhpps");
    EXPECT_TRUE(rejected.isError());
    EXPECT_EQ(rejected.property("lineNumber").toInt(), 2);
    EXPECT_TRUE(rejected.toString().contains("directory must be empty"));
    EXPECT_EQ(scene->scriptRunRayCount().value_or(0), 128u);
    const auto accepted = fixture.host.evaluate("tn.Run();", "run-recovered.tnhpps");
    EXPECT_FALSE(accepted.isError()) << accepted.toString().toStdString();
    EXPECT_EQ(calls, 2);
}

TEST(ScientificScriptRunDispatch, RequestsAreIsolatedBetweenHosts)
{
    TonatiuhCore::initializeCoin();
    LegacySceneFixture a;
    LegacySceneFixture b;
    ASSERT_FALSE(a.host.evaluate("tn.Clear();tn.SetRaysNumber(8192);").isError());
    ASSERT_FALSE(b.host.evaluate("tn.Clear();").isError());
    auto* sa = qobject_cast<ScriptSceneServices*>(a.host.engine()
        ->globalObject().property("__tonatiuhppSceneApi").toQObject());
    auto* sb = qobject_cast<ScriptSceneServices*>(b.host.engine()
        ->globalObject().property("__tonatiuhppSceneApi").toQObject());
    ASSERT_NE(sa, nullptr);
    ASSERT_NE(sb, nullptr);
    sa->setRunExecutor([&](const ScriptRunRequest& r, QString*) {
        EXPECT_EQ(r.rays, 8192u);
        EXPECT_TRUE(r.explicitRays.has_value());
        return true;
    });
    sb->setRunExecutor([&](const ScriptRunRequest& r, QString*) {
        EXPECT_EQ(r.rays, 10000u); // historical GUI constructor default
        EXPECT_FALSE(r.explicitRays.has_value());
        return true;
    });
    EXPECT_FALSE(a.host.evaluate("tn.Run();").isError());
    EXPECT_FALSE(b.host.evaluate("tn.Run();").isError());
}

TEST(ScientificScriptAppendDispatch, RepeatedCallsKeepExplicitAppendRequest)
{
    TonatiuhCore::initializeCoin();
    LegacySceneFixture fixture;
    ASSERT_FALSE(fixture.host.evaluate("tn.Clear();").isError());
    auto* scene = qobject_cast<ScriptSceneServices*>(fixture.host.engine()
        ->globalObject().property("__tonatiuhppSceneApi").toQObject());
    ASSERT_NE(scene, nullptr);
    int runs = 0;
    scene->setRunExecutor([&](const ScriptRunRequest& request, QString*) {
        ++runs;
        EXPECT_EQ(request.rays, 2048u);
        EXPECT_TRUE(request.exportOptions.bufferAppend.value_or(false));
        EXPECT_EQ(request.exportOptions.bufferSize.value_or(0), 4096u);
        EXPECT_EQ(request.exportOptions.exporterName.value_or(""), "File");
        return true;
    });
    const auto result = fixture.host.evaluate(R"JS(
        tn.SetRaysNumber(2048);
        tn.SetExportPhotonMapType('File');
        tn.SetPhotonBufferSize(4096);
        tn.SetPhotonBufferAppend(true);
        tn.Run();
        tn.Run();
    )JS", "run-append-request.tnhpps");
    EXPECT_FALSE(result.isError()) << result.toString().toStdString();
    EXPECT_EQ(runs, 2);
}

TEST(ScientificScriptAppendDispatch, RejectedSecondCallReportsUserLineAndHostRecovers)
{
    TonatiuhCore::initializeCoin();
    LegacySceneFixture fixture;
    ASSERT_FALSE(fixture.host.evaluate("tn.Clear();tn.SetRaysNumber(128);tn.SetPhotonBufferAppend(true);").isError());
    auto* scene = qobject_cast<ScriptSceneServices*>(fixture.host.engine()
        ->globalObject().property("__tonatiuhppSceneApi").toQObject());
    ASSERT_NE(scene, nullptr);
    int calls = 0;
    scene->setRunExecutor([&](const ScriptRunRequest& request, QString* error) {
        ++calls;
        EXPECT_TRUE(request.exportOptions.bufferAppend.value_or(false));
        if (calls == 2) {
            if (error) *error = "tn.Run: existing File export changed externally";
            return false;
        }
        return true;
    });
    const auto first = fixture.host.evaluate("tn.Run();", "append-first.tnhpps");
    EXPECT_FALSE(first.isError()) << first.toString().toStdString();
    const auto second = fixture.host.evaluate(
        "var marker = 1;\ntn.Run();", "append-rejected.tnhpps");
    EXPECT_TRUE(second.isError());
    EXPECT_EQ(second.property("lineNumber").toInt(), 2);
    EXPECT_TRUE(second.toString().contains("changed externally"));
    const auto third = fixture.host.evaluate("tn.Run();", "append-recovery.tnhpps");
    EXPECT_FALSE(third.isError()) << third.toString().toStdString();
    EXPECT_EQ(calls, 3);
}

// Phase 3B2B2B Slice A: deterministic, in-memory exporter lifecycle failures.
// No filesystem or ray-tracing side effects are needed to exercise the latch.
namespace
{
class M3FailingPhotonExporter final : public PhotonsAbstract
{
public:
    bool allowStart = true;
    bool allowFinish = true;
    bool reportError = false;
    int starts = 0;
    int finishes = 0;
    double lastPower = 0.;

    bool startExport() override { ++starts; return allowStart; }
    bool endExport() override { ++finishes; return allowFinish; }
    bool hasExportError() const override { return reportError; }
    void setPhotonPower(double power) override { lastPower = power; }
};
} // namespace

TEST(ScientificPhotonBufferFailureLatch, FailedFinalizationLatchesFailure)
{
    PhotonsBuffer buffer(16);
    M3FailingPhotonExporter exporter;
    exporter.allowFinish = false;
    ASSERT_TRUE(buffer.setExporter(&exporter));
    EXPECT_FALSE(buffer.endExport(0.125));
    EXPECT_TRUE(buffer.hasExportFailed());
    EXPECT_EQ(exporter.starts, 1);
    EXPECT_EQ(exporter.finishes, 1);
    EXPECT_DOUBLE_EQ(exporter.lastPower, 0.125);
    EXPECT_FALSE(buffer.addPhotons({}));
}

TEST(ScientificPhotonBufferFailureLatch, ExporterErrorAfterFinalizationLatchesFailure)
{
    PhotonsBuffer buffer(16);
    M3FailingPhotonExporter exporter;
    ASSERT_TRUE(buffer.setExporter(&exporter));
    exporter.reportError = true;
    EXPECT_FALSE(buffer.endExport(0.25));
    EXPECT_TRUE(buffer.hasExportFailed());
    EXPECT_EQ(exporter.finishes, 1);
}

TEST(ScientificPhotonBufferFailureLatch, SuccessfulFinalizationKeepsHealthyState)
{
    PhotonsBuffer buffer(16);
    M3FailingPhotonExporter exporter;
    ASSERT_TRUE(buffer.setExporter(&exporter));
    EXPECT_TRUE(buffer.endExport(0.5));
    EXPECT_FALSE(buffer.hasExportFailed());
    EXPECT_EQ(exporter.finishes, 1);
    EXPECT_DOUBLE_EQ(exporter.lastPower, 0.5);
}

// M3 Step 3B: verify the GUI-to-shared-session input mapping without widgets.
TEST(ScientificGuiTraceRunInputs, CopiesResolvedScientificConfiguration)
{
    const auto input = GuiTraceRunInputs::make(
        nullptr, nullptr, nullptr, nullptr, {}, nullptr,
        std::uint64_t{123456789}, 2048ul, 13, 17);
    EXPECT_EQ(input.configuration.masterSeed, std::uint64_t{123456789});
    EXPECT_EQ(input.configuration.rays, 2048ul);
    EXPECT_EQ(input.configuration.sunWidthDivisions, 13);
    EXPECT_EQ(input.configuration.sunHeightDivisions, 17);
    EXPECT_EQ(input.scene, nullptr);
    EXPECT_EQ(input.photonBuffer, nullptr);
    EXPECT_TRUE(input.exportSurfaceList.empty());
    EXPECT_FALSE(static_cast<bool>(input.hitCallback));
    EXPECT_FALSE(static_cast<bool>(input.historyCallback));
}

TEST(ScientificGuiTraceRunInputs, PreservesExplicitSurfaceSelectionAndDefaultInputs)
{
    QVector<InstanceNode*> surfaces;
    surfaces << nullptr << nullptr;
    const auto input = GuiTraceRunInputs::make(
        nullptr, nullptr, nullptr, nullptr, surfaces, nullptr,
        std::uint64_t{0}, 0ul, 0, 0);
    ASSERT_EQ(input.exportSurfaceList.size(), 2);
    EXPECT_EQ(input.exportSurfaceList[0], nullptr);
    EXPECT_EQ(input.exportSurfaceList[1], nullptr);
    EXPECT_EQ(input.configuration.masterSeed, std::uint64_t{0});
    EXPECT_EQ(input.configuration.rays, 0ul);
    EXPECT_EQ(input.configuration.sunWidthDivisions, 0);
    EXPECT_EQ(input.configuration.sunHeightDivisions, 0);
    EXPECT_EQ(input.tracingAir, nullptr);
}

// M3 Step 3A: regression-lock the nonvisual GUI completion policy.
TEST(ScientificGuiTraceCompletion, SuccessAccumulatesRaysAndNormalizesPower)
{
    M3FailingPhotonExporter exporter;
    PhotonsBuffer buffer(16);
    ASSERT_TRUE(buffer.setExporter(&exporter));
    const auto result = GuiTraceCompletion::finalize(buffer, true, false,
                                                     100ul, 50ul, 3., 10.);
    EXPECT_EQ(result.raysTracedTotal, 150ul);
    EXPECT_DOUBLE_EQ(result.photonPower, 0.2);
    EXPECT_TRUE(result.exportSucceeded);
    EXPECT_TRUE(result.accepted);
    EXPECT_DOUBLE_EQ(exporter.lastPower, 0.2);
}

TEST(ScientificGuiTraceCompletion, IncompleteTraceNeverNormalizesPartialPhotons)
{
    M3FailingPhotonExporter exporter;
    PhotonsBuffer buffer(16);
    ASSERT_TRUE(buffer.setExporter(&exporter));
    const auto result = GuiTraceCompletion::finalize(buffer, false, false,
                                                     100ul, 50ul, 3., 10.);
    EXPECT_EQ(result.raysTracedTotal, 100ul);
    EXPECT_DOUBLE_EQ(result.photonPower, 0.3);
    EXPECT_TRUE(result.exportSucceeded);
    EXPECT_FALSE(result.accepted);
    EXPECT_DOUBLE_EQ(exporter.lastPower, 0.);
}

TEST(ScientificGuiTraceCompletion, ExportFailureNeverAcceptsCompletedTrace)
{
    M3FailingPhotonExporter exporter;
    PhotonsBuffer buffer(16);
    ASSERT_TRUE(buffer.setExporter(&exporter));
    exporter.allowFinish = false;
    const auto result = GuiTraceCompletion::finalize(buffer, true, false,
                                                     0ul, 50ul, 3., 10.);
    EXPECT_EQ(result.raysTracedTotal, 50ul);
    EXPECT_DOUBLE_EQ(result.photonPower, 0.6);
    EXPECT_FALSE(result.exportSucceeded);
    EXPECT_FALSE(result.accepted);
    EXPECT_TRUE(buffer.hasExportFailed());
    EXPECT_DOUBLE_EQ(exporter.lastPower, 0.6);
}

TEST(ScientificGuiTraceCompletion, WorkerExportFailureRejectsEvenIfFinalizationSucceeds)
{
    M3FailingPhotonExporter exporter;
    PhotonsBuffer buffer(16);
    ASSERT_TRUE(buffer.setExporter(&exporter));
    const auto result = GuiTraceCompletion::finalize(buffer, true, true,
                                                     0ul, 50ul, 3., 10.);
    EXPECT_EQ(result.raysTracedTotal, 50ul);
    EXPECT_TRUE(result.exportSucceeded);
    EXPECT_FALSE(result.accepted);
}


// Phase 3B2B2B Slice B: exercise the actual production File exporter plugin.
// The test injects a deterministic metadata-open failure using a directory at
// the metadata filename. Only QTemporaryDir contents are touched.
namespace
{
struct M3LoadedFileExporter
{
    QPluginLoader loader;
    std::unique_ptr<PhotonsAbstract> exporter;

    M3LoadedFileExporter()
    {
        loader.setFileName(
            QString::fromUtf8(TONATIUHPP_SCIENTIFIC_FILE_PLUGIN));

        QObject* object = loader.instance();
        auto* factory = qobject_cast<PhotonsFactory*>(object);

        if (factory && factory->name() == "File") {
            exporter.reset(factory->create(0));
        }
    }
};
} // namespace

TEST(ScientificPhotonsFileMetadataFailure, MetadataOpenFailureLatchesExporter)
{
    QTemporaryDir temp;
    ASSERT_TRUE(temp.isValid());
    M3LoadedFileExporter loaded;
    ASSERT_NE(loaded.exporter, nullptr) << "Production PhotonsFile plugin is unavailable";
    loaded.exporter->setParameter("ExportDirectory", temp.path());
    loaded.exporter->setParameter("ExportFile", "PhotonMap");
    loaded.exporter->setParameter("FileSize", "-1");
    ASSERT_TRUE(loaded.exporter->startExport());
    const QString collision = QDir(temp.path()).filePath("PhotonMap_parameters.txt");
    ASSERT_TRUE(QDir().mkpath(collision));
    EXPECT_FALSE(loaded.exporter->endExport());
    EXPECT_TRUE(loaded.exporter->hasExportError());
    EXPECT_TRUE(QFileInfo(collision).isDir());
    EXPECT_FALSE(QFileInfo(QDir(temp.path()).filePath("PhotonMap.dat")).exists());
}

TEST(ScientificPhotonsFileMetadataFailure, SuccessfulMetadataFinalizationStaysHealthy)
{
    QTemporaryDir temp;
    ASSERT_TRUE(temp.isValid());
    M3LoadedFileExporter loaded;
    ASSERT_NE(loaded.exporter, nullptr) << "Production PhotonsFile plugin is unavailable";
    loaded.exporter->setParameter("ExportDirectory", temp.path());
    loaded.exporter->setParameter("ExportFile", "PhotonMap");
    loaded.exporter->setParameter("FileSize", "-1");
    ASSERT_TRUE(loaded.exporter->startExport());
    loaded.exporter->setPhotonPower(0.125);
    EXPECT_TRUE(loaded.exporter->endExport());
    EXPECT_FALSE(loaded.exporter->hasExportError());
    QFile file(QDir(temp.path()).filePath("PhotonMap_parameters.txt"));
    ASSERT_TRUE(file.open(QIODevice::ReadOnly));
    EXPECT_TRUE(file.readAll().contains("START PARAMETERS"));
}

// Phase 3B2B2B Slice C: deterministic data-file failure injection, exercised
// through the real production File plugin, not a replacement serializer.
namespace {
struct M3ScientificDataFaultScope
{
    QByteArray old = qgetenv("TONATIUHPP_M3_TEST_FILE_DATA_FAULT");
    bool existed = qEnvironmentVariableIsSet("TONATIUHPP_M3_TEST_FILE_DATA_FAULT");
    explicit M3ScientificDataFaultScope(const char* mode)
    {
        qputenv("TONATIUHPP_M3_TEST_FILE_DATA_FAULT", QByteArray(mode));
    }
    ~M3ScientificDataFaultScope()
    {
        if (existed) qputenv("TONATIUHPP_M3_TEST_FILE_DATA_FAULT", old);
        else qunsetenv("TONATIUHPP_M3_TEST_FILE_DATA_FAULT");
    }
};

void m3VerifyInjectedDataFailure(const char* mode)
{
    QTemporaryDir temp;
    ASSERT_TRUE(temp.isValid());
    M3LoadedFileExporter loaded;
    ASSERT_NE(loaded.exporter, nullptr) << "Production PhotonsFile plugin is unavailable";
    loaded.exporter->setParameter("ExportDirectory", temp.path());
    loaded.exporter->setParameter("ExportFile", "PhotonMap");
    loaded.exporter->setParameter("FileSize", "-1");
    ASSERT_TRUE(loaded.exporter->startExport());
    const QString dataPath = QDir(temp.path()).filePath("PhotonMap.dat");
    {
        M3ScientificDataFaultScope fault(mode);
        // Use the real constructor: Photon() leaves pointer members uninitialized.
        std::vector<Photon> photons;
        photons.emplace_back(1, vec3d{}, nullptr, true, false);
        EXPECT_EQ(loaded.exporter->savePhotons(photons), 0ul);
        EXPECT_TRUE(loaded.exporter->hasExportError());
    }
    EXPECT_FALSE(loaded.exporter->endExport());
    EXPECT_TRUE(loaded.exporter->hasExportError());
    EXPECT_TRUE(QFileInfo::exists(dataPath));
    EXPECT_EQ(QFileInfo(dataPath).size(), 0);
    EXPECT_FALSE(QFileInfo::exists(QDir(temp.path()).filePath("PhotonMap_parameters.txt")));
}
} // namespace

TEST(ScientificPhotonsFileDataFailure, InjectedWriteFailureLatchesAndRejectsFinalization)
{
    m3VerifyInjectedDataFailure("write");
}

TEST(ScientificPhotonsFileDataFailure, InjectedFlushFailureLatchesAndRejectsFinalization)
{
    m3VerifyInjectedDataFailure("flush");
}


// Phase 3B2B2B Slice D1: File append failure containment at the real
// PhotonsBuffer / production PhotonsFile boundary. Host-level tn.Run
// poisoning and scene ownership still require separate acceptance.
namespace {
void m3VerifyFailedFileAppendRetainsOwnership(const char* faultMode)
{
    QTemporaryDir temp;
    ASSERT_TRUE(temp.isValid());
    M3LoadedFileExporter loaded;
    ASSERT_NE(loaded.exporter, nullptr) << "Production PhotonsFile plugin is unavailable";
    loaded.exporter->setParameter("ExportDirectory", temp.path());
    loaded.exporter->setParameter("ExportFile", "PhotonMap");
    loaded.exporter->setParameter("FileSize", "-1");

    // The exporter must outlive the buffer, which borrows its pointer.
    PhotonsBuffer buffer(1);
    ASSERT_TRUE(buffer.setExporter(loaded.exporter.get()));
    std::vector<Photon> photons;
    photons.emplace_back(1, vec3d{}, nullptr, true, false);
    ASSERT_TRUE(buffer.addPhotons(photons));
    ASSERT_TRUE(buffer.endExport(0.125));
    EXPECT_FALSE(buffer.hasExportFailed());
    EXPECT_FALSE(buffer.hasRetainedPhotons());

    const QString dataPath = QDir(temp.path()).filePath("PhotonMap.dat");
    const QString paramsPath = QDir(temp.path()).filePath("PhotonMap_parameters.txt");
    QFile initialData(dataPath);
    ASSERT_TRUE(initialData.open(QIODevice::ReadOnly));
    const QByteArray originalData = initialData.readAll();
    initialData.close();
    ASSERT_FALSE(originalData.isEmpty());
    QFile initialParams(paramsPath);
    ASSERT_TRUE(initialParams.open(QIODevice::ReadOnly));
    const QByteArray originalParams = initialParams.readAll();
    initialParams.close();
    ASSERT_FALSE(originalParams.isEmpty());

    // A successful File export establishes an append checkpoint.
    ASSERT_TRUE(loaded.exporter->startExport());
    ASSERT_TRUE(buffer.addPhotons(photons));
    ASSERT_TRUE(buffer.hasRetainedPhotons());
    {
        M3ScientificDataFaultScope fault(faultMode);
        EXPECT_FALSE(buffer.endExport(0.125));
    }
    EXPECT_TRUE(buffer.hasExportFailed());
    EXPECT_TRUE(loaded.exporter->hasExportError());
    ASSERT_TRUE(buffer.hasRetainedPhotons());
    EXPECT_EQ(buffer.getPhotons().size(), 1u);
    EXPECT_FALSE(buffer.addPhotons(photons));
    EXPECT_FALSE(loaded.exporter->startExport());

    // Rollback must preserve both previously completed append checkpoint files.
    QFile finalData(dataPath);
    ASSERT_TRUE(finalData.open(QIODevice::ReadOnly));
    EXPECT_EQ(finalData.readAll(), originalData);
    QFile finalParams(paramsPath);
    ASSERT_TRUE(finalParams.open(QIODevice::ReadOnly));
    EXPECT_EQ(finalParams.readAll(), originalParams);
}
} // namespace

TEST(ScientificPhotonsFileAppendFailure, WriteFailurePreservesCheckpointAndRetainsBuffer)
{
    m3VerifyFailedFileAppendRetainsOwnership("write");
}

TEST(ScientificPhotonsFileAppendFailure, FlushFailurePreservesCheckpointAndRetainsBuffer)
{
    m3VerifyFailedFileAppendRetainsOwnership("flush");
}

// M4 finishing: actual production File plugin with segmented output, the
// existing scientific-executable-only write/flush faults, and partial-run
// checkpoint invalidation. All tests use independent QTemporaryDir roots.
namespace {
QByteArray m3ReadExportFile(const QString& path);
std::vector<Photon> m4Photons(std::size_t count)
{
    std::vector<Photon> photons;
    photons.reserve(count);
    for (std::size_t i = 0; i < count; ++i)
        photons.emplace_back(1, vec3d{}, nullptr, true, false);
    return photons;
}

void m4VerifySegmentedAppendFault(const char* faultMode)
{
    QTemporaryDir temp;
    ASSERT_TRUE(temp.isValid());
    M3LoadedFileExporter loaded;
    ASSERT_NE(loaded.exporter, nullptr);
    loaded.exporter->setParameter("ExportDirectory", temp.path());
    loaded.exporter->setParameter("ExportFile", "PhotonMap");
    loaded.exporter->setParameter("FileSize", "2");
    PhotonsBuffer buffer(2);
    ASSERT_TRUE(buffer.setExporter(loaded.exporter.get()));
    ASSERT_TRUE(buffer.addPhotons(m4Photons(2)));
    ASSERT_TRUE(buffer.endExport(0.125));

    const QDir dir(temp.path());
    const QString first = dir.filePath("PhotonMap_1.dat");
    const QString second = dir.filePath("PhotonMap_2.dat");
    const QString metadata = dir.filePath("PhotonMap_parameters.txt");
    const QByteArray beforeFirst = m3ReadExportFile(first);
    const QByteArray beforeMetadata = m3ReadExportFile(metadata);
    ASSERT_FALSE(beforeFirst.isEmpty());
    ASSERT_FALSE(beforeMetadata.isEmpty());
    ASSERT_FALSE(QFileInfo::exists(second));

    ASSERT_TRUE(loaded.exporter->startExport());
    ASSERT_TRUE(buffer.addPhotons(m4Photons(1)));
    {
        M3ScientificDataFaultScope failure(faultMode);
        EXPECT_FALSE(buffer.endExport(0.125));
    }
    EXPECT_TRUE(buffer.hasExportFailed());
    EXPECT_TRUE(buffer.hasRetainedPhotons());
    EXPECT_TRUE(loaded.exporter->hasExportError());
    EXPECT_EQ(buffer.getPhotons().size(), 1u);
    EXPECT_EQ(m3ReadExportFile(first), beforeFirst);
    EXPECT_EQ(m3ReadExportFile(metadata), beforeMetadata);
    EXPECT_TRUE(QFileInfo::exists(second));
    EXPECT_EQ(QFileInfo(second).size(), 0);
    EXPECT_FALSE(loaded.exporter->startExport());
    EXPECT_FALSE(buffer.addPhotons(m4Photons(1)));
}
} // namespace

TEST(ScientificM4SegmentedExport, InjectedAppendWriteFailurePreservesPriorFiles)
{
    m4VerifySegmentedAppendFault("write");
}

TEST(ScientificM4SegmentedExport, InjectedAppendFlushFailurePreservesPriorFiles)
{
    m4VerifySegmentedAppendFault("flush");
}

TEST(ScientificM4SegmentedExport, InterruptedAppendInvalidatesOutputCheckpoint)
{
    QTemporaryDir temp;
    ASSERT_TRUE(temp.isValid());
    M3LoadedFileExporter loaded;
    ASSERT_NE(loaded.exporter, nullptr);
    loaded.exporter->setParameter("ExportDirectory", temp.path());
    loaded.exporter->setParameter("ExportFile", "PhotonMap");
    loaded.exporter->setParameter("FileSize", "2");
    ASSERT_TRUE(loaded.exporter->startExport());
    ASSERT_EQ(loaded.exporter->savePhotons(m4Photons(2)), 2ul);
    loaded.exporter->setPhotonPower(0.125);
    ASSERT_TRUE(loaded.exporter->endExport());
    QMap<QString, QByteArray> accepted;
    bool hasData = false;
    QString error;
    ASSERT_TRUE(PhotonExportFileService::captureOutputCheckpoint(
        temp.path(), "PhotonMap", &accepted, &hasData, &error)) << error.toStdString();
    ASSERT_TRUE(hasData);
    const QString metadata = QDir(temp.path()).filePath("PhotonMap_parameters.txt");
    const QByteArray beforeMetadata = m3ReadExportFile(metadata);
    ASSERT_FALSE(beforeMetadata.isEmpty());
    ASSERT_TRUE(loaded.exporter->startExport());
    ASSERT_EQ(loaded.exporter->savePhotons(m4Photons(1)), 1ul);
    // Simulated interruption: destructor closes the data file but no
    // endExport/metadata finalization occurs. No live handles impede the
    // checkpoint reader on Windows.
    loaded.exporter.reset();
    QMap<QString, QByteArray> interrupted;
    ASSERT_TRUE(PhotonExportFileService::captureOutputCheckpoint(
        temp.path(), "PhotonMap", &interrupted, &hasData, &error)) << error.toStdString();
    EXPECT_TRUE(hasData);
    EXPECT_NE(interrupted, accepted);
    EXPECT_EQ(m3ReadExportFile(metadata), beforeMetadata);
}

TEST(ScientificM4SegmentedExport, UnrelatedWildcardFilesAreNeverDeleted)
{
    QTemporaryDir temp;
    ASSERT_TRUE(temp.isValid());
    const QDir dir(temp.path());
    const QString unrelated = dir.filePath("PhotonMap_notes.dat");
    const QString invalidSerial = dir.filePath("PhotonMap_0.dat");
    const QString managed = dir.filePath("PhotonMap_1.dat");
    for (const QString& path : {unrelated, invalidSerial, managed}) {
        QFile file(path);
        ASSERT_TRUE(file.open(QIODevice::WriteOnly));
        ASSERT_EQ(file.write("sentinel"), 8);
    }
    M3LoadedFileExporter loaded;
    ASSERT_NE(loaded.exporter, nullptr);
    loaded.exporter->setParameter("ExportDirectory", temp.path());
    loaded.exporter->setParameter("ExportFile", "PhotonMap");
    loaded.exporter->setParameter("FileSize", "2");
    ASSERT_TRUE(loaded.exporter->startExport());
    EXPECT_EQ(m3ReadExportFile(unrelated), QByteArray("sentinel"));
    EXPECT_EQ(m3ReadExportFile(invalidSerial), QByteArray("sentinel"));
    EXPECT_FALSE(QFileInfo::exists(managed));
}

TEST(ScientificM4SegmentedExport, CheckpointDetectsNewUnrelatedEntry)
{
    QTemporaryDir temp;
    ASSERT_TRUE(temp.isValid());
    M3LoadedFileExporter loaded;
    ASSERT_NE(loaded.exporter, nullptr);
    loaded.exporter->setParameter("ExportDirectory", temp.path());
    loaded.exporter->setParameter("ExportFile", "PhotonMap");
    loaded.exporter->setParameter("FileSize", "2");
    ASSERT_TRUE(loaded.exporter->startExport());
    ASSERT_EQ(loaded.exporter->savePhotons(m4Photons(2)), 2ul);
    loaded.exporter->setPhotonPower(0.125);
    ASSERT_TRUE(loaded.exporter->endExport());
    QMap<QString, QByteArray> accepted;
    bool hasData = false;
    QString error;
    ASSERT_TRUE(PhotonExportFileService::captureOutputCheckpoint(
        temp.path(), "PhotonMap", &accepted, &hasData, &error));
    ASSERT_TRUE(hasData);
    QFile injected(QDir(temp.path()).filePath("foreign_notes.dat"));
    ASSERT_TRUE(injected.open(QIODevice::WriteOnly));
    ASSERT_EQ(injected.write("new"), 3);
    injected.close();
    QMap<QString, QByteArray> observed;
    ASSERT_TRUE(PhotonExportFileService::captureOutputCheckpoint(
        temp.path(), "PhotonMap", &observed, &hasData, &error));
    EXPECT_NE(observed, accepted);
}


// M3 Phase 3B2B2B D2: exercise the genuine per-ScriptHost File backend through
// the script-facing tn.Run() API; no substitute exporter or mock run executor.
namespace {
void m3ConfigureRealFileRun(LegacySceneFixture& fixture, const QString& directory)
{
    const QString script = QStringLiteral(R"JS(
        tn.SetRaysNumber(2048);
        tn.SetRaysGrid(200, 200);
        tn.SetExportPhotonMapType('File');
        tn.SetExportCoordinates(true, true);
        tn.SetExportIntersectionSurface(true);
        tn.SetExportIntersectionSurfaceSide(true);
        tn.SetExportPreviousNextPhotonID(false);
        tn.SetPhotonBufferSize(1);
        tn.SetPhotonBufferAppend(true);
        tn.SetExportTypeParameterValue('ExportDirectory', %1);
        tn.SetExportTypeParameterValue('ExportFile', 'PhotonMap');
        tn.SetExportTypeParameterValue('FileSize', '-1');
        tn.AddExportSurfaceURL('//Node/Shape');
    )JS").arg(QString::fromUtf8(QJsonDocument(QJsonArray{directory}).toJson(QJsonDocument::Compact)).mid(1).chopped(1));
    const QJSValue result = fixture.host.evaluate(script, "d2-configure.tnhpps");
    ASSERT_FALSE(result.isError()) << result.toString().toStdString();
}

QByteArray m3ReadExportFile(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return {};
    return file.readAll();
}

void m3VerifyRealHostPoisoning(const char* mode)
{
    TonatiuhCore::initializeCoin();
    QTemporaryDir output;
    ASSERT_TRUE(output.isValid());
    LegacySceneFixture fixture;
    QString sceneError;
    ASSERT_TRUE(fixture.document.load(QString::fromUtf8(TONATIUHPP_EQUIVALENCE_SCENE_FILE),
                                      &sceneError)) << sceneError.toStdString();
    auto backend = std::make_shared<HeadlessScriptRunFileBackend>();
    auto* sceneApi = qobject_cast<ScriptSceneServices*>(fixture.host.engine()->globalObject()
        .property("__tonatiuhppSceneApi").toQObject());
    ASSERT_NE(sceneApi, nullptr);
    int backendCalls = 0;
    sceneApi->setRunExecutor([backend, &backendCalls](const ScriptRunRequest& request,
                                                      QString* error) {
        ++backendCalls;
        return backend->run(request, error);
    });
    m3ConfigureRealFileRun(fixture, output.path());
    const QJSValue firstRun = fixture.host.evaluate("tn.Run();", "d2-first.tnhpps");
    ASSERT_FALSE(firstRun.isError()) << firstRun.toString().toStdString()
        << " [backendCalls=" << backendCalls << "]";
    const QString base = QDir(output.path()).filePath("PhotonMap");
    const QByteArray previousData = m3ReadExportFile(base + ".dat");
    const QByteArray previousParams = m3ReadExportFile(base + "_parameters.txt");
    ASSERT_FALSE(previousData.isEmpty());
    ASSERT_FALSE(previousParams.isEmpty());
    ASSERT_EQ(backendCalls, 1);

    {
        M3ScientificDataFaultScope fault(mode);
        const QJSValue failure = fixture.host.evaluate(
            "var d2Marker = 1;\ntn.Run();", "d2-append-failure.tnhpps");
        ASSERT_TRUE(failure.isError());
        EXPECT_EQ(failure.property("lineNumber").toInt(), 2);
        EXPECT_TRUE(failure.toString().contains("Photon buffer/export failed during ray tracing"))
            << failure.toString().toStdString();
    }
    EXPECT_EQ(backendCalls, 2);
    EXPECT_EQ(m3ReadExportFile(base + ".dat"), previousData);
    EXPECT_EQ(m3ReadExportFile(base + "_parameters.txt"), previousParams);

    const QJSValue blocked = fixture.host.evaluate(
        "tn.Run();", "d2-poisoned-append.tnhpps");
    ASSERT_TRUE(blocked.isError());
    EXPECT_TRUE(blocked.toString().contains("previous export failed"));
    EXPECT_EQ(backendCalls, 3);

    // Clearing the script document drops its own scene reference, but the
    // poisoned backend still pins exporter, buffer and original Coin scene.
    ASSERT_TRUE(fixture.document.clear(&sceneError)) << sceneError.toStdString();
    QTemporaryDir fresh;
    ASSERT_TRUE(fresh.isValid());
    const QString resetScript = QStringLiteral(
        "tn.SetPhotonBufferAppend(false);\n"
        "tn.SetExportTypeParameterValue('ExportDirectory', %1);\n"
        "tn.Run();").arg(QString::fromUtf8(
            QJsonDocument(QJsonArray{fresh.path()}).toJson(QJsonDocument::Compact)).mid(1).chopped(1));
    const QJSValue reset = fixture.host.evaluate(resetScript, "d2-poisoned-reset.tnhpps");
    ASSERT_TRUE(reset.isError());
    EXPECT_EQ(reset.property("lineNumber").toInt(), 3);
    EXPECT_TRUE(reset.toString().contains("previous export failed"));
    EXPECT_EQ(backendCalls, 4);
    EXPECT_TRUE(QDir(fresh.path()).entryList(QDir::Files | QDir::NoDotAndDotDot).isEmpty());

    // Independent ScriptHost and backend are not poisoned by this one.
    LegacySceneFixture independent;
    ASSERT_TRUE(independent.document.load(
        QString::fromUtf8(TONATIUHPP_EQUIVALENCE_SCENE_FILE), &sceneError));
    auto independentBackend = std::make_shared<HeadlessScriptRunFileBackend>();
    auto* secondApi = qobject_cast<ScriptSceneServices*>(independent.host.engine()
        ->globalObject().property("__tonatiuhppSceneApi").toQObject());
    ASSERT_NE(secondApi, nullptr);
    secondApi->setRunExecutor([independentBackend](const ScriptRunRequest& request,
                                                   QString* error) {
        return independentBackend->run(request, error);
    });
    const QJSValue clean = independent.host.evaluate(
        "tn.SetRaysNumber(128); tn.SetRaysGrid(200, 200); tn.Run();",
        "d2-independent-host.tnhpps");
    EXPECT_FALSE(clean.isError()) << clean.toString().toStdString();
}
} // namespace

TEST(ScientificScriptFileHostPoison, AppendWriteFailureBlocksResetAndPreservesOwnership)
{
    m3VerifyRealHostPoisoning("write");
}

TEST(ScientificScriptFileHostPoison, AppendFlushFailureBlocksResetAndPreservesOwnership)
{
    m3VerifyRealHostPoisoning("flush");
}

// M3 final scripted Run migration: no GUI-dependent scientific implementation.
TEST(ScientificSharedScriptRun, OptionalExplicitSeedIsValidatedAtUserCallSite)
{
    TonatiuhCore::initializeCoin();
    LegacySceneFixture fixture;
    int calls = 0;
    std::optional<std::uint64_t> lastSeed;
    auto* services = qobject_cast<ScriptSceneServices*>(fixture.host.engine()
        ->globalObject().property("__tonatiuhppSceneApi").toQObject());
    ASSERT_NE(services, nullptr);
    services->setRunExecutor([&](const ScriptRunRequest& request, QString*) {
        ++calls;
        lastSeed = request.masterSeed;
        return true;
    });
    ASSERT_TRUE(fixture.document.clear());
    const QJSValue seeded = fixture.host.evaluate(
        "tn.SetRaysNumber(128);\ntn.Run(0);", "m3-run-seed-zero.tnhpps");
    ASSERT_FALSE(seeded.isError()) << seeded.toString().toStdString();
    EXPECT_EQ(calls, 1);
    ASSERT_TRUE(lastSeed.has_value());
    EXPECT_EQ(*lastSeed, 0u);
    const QJSValue unseeded = fixture.host.evaluate(
        "tn.Run();", "m3-run-legacy.tnhpps");
    ASSERT_FALSE(unseeded.isError()) << unseeded.toString().toStdString();
    EXPECT_EQ(calls, 2);
    EXPECT_FALSE(lastSeed.has_value());

    const QJSValue invalid = fixture.host.evaluate(
        "var a = 1;\ntn.Run(0.5);", "m3-run-bad-seed.tnhpps");
    ASSERT_TRUE(invalid.isError());
    EXPECT_EQ(invalid.property("lineNumber").toInt(), 2);
    EXPECT_TRUE(invalid.toString().contains("uint32"));
    EXPECT_EQ(calls, 2);
    const QJSValue overflow = fixture.host.evaluate(
        "tn.Run(4294967296);", "m3-run-overflow.tnhpps");
    ASSERT_TRUE(overflow.isError());
    EXPECT_EQ(calls, 2);
}

TEST(ScientificSharedScriptRun, RealNonvisualExecutorRepeatsWithEffectiveSeeds)
{
    TonatiuhCore::initializeCoin();
    LegacySceneFixture fixture;
    QString error;
    ASSERT_TRUE(fixture.document.load(
        QString::fromUtf8(TONATIUHPP_EQUIVALENCE_SCENE_FILE), &error))
        << error.toStdString();
    ScriptRunRequest request;
    request.scene = fixture.document.scene();
    request.explicitRays = 128u;
    request.rays = 128;
    request.sunWidthDivisions = 64;
    request.sunHeightDivisions = 64;
    ScriptRunExecutionService backend;
    ASSERT_TRUE(backend.run(request, 0u, &error)) << error.toStdString();
    ASSERT_TRUE(backend.run(request, 123456789u, &error)) << error.toStdString();
    request.masterSeed = 0u;
    ASSERT_TRUE(backend.run(request, 123456789u, &error)) << error.toStdString();
}

TEST(ScientificSharedScriptRun, MissingInputsRejectWithoutCreatingExport)
{
    ScriptRunExecutionService backend;
    ScriptRunRequest request;
    QString error;
    EXPECT_FALSE(backend.run(request, 0u, &error));
    EXPECT_TRUE(error.contains("active scene"));
    TonatiuhCore::initializeCoin();
    LegacySceneFixture fixture;
    ASSERT_TRUE(fixture.document.clear(&error)) << error.toStdString();
    request.scene = fixture.document.scene();
    EXPECT_FALSE(backend.run(request, 0u, &error));
    EXPECT_TRUE(error.contains("effective ray count"));
    // No exporter, file or instance tree is opened for this rejected call.
}
