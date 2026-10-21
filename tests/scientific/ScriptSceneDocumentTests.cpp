#include <gtest/gtest.h>

#include <Inventor/nodes/SoGroup.h>

#include <QJSEngine>

#include "core/CorePluginRegistry.h"
#include "core/TonatiuhCore.h"
#include "kernel/scene/TSceneKit.h"
#include "kernel/scene/TSeparatorKit.h"
#include "script/ScriptHost.h"
#include "script/ScriptSceneDocument.h"
#include "script/ScriptSceneServices.h"

namespace
{
struct ScriptSceneFixture
{
    ScriptHost host;
    ScriptSceneDocument document;
    ScriptSceneServices* services = nullptr;

    ScriptSceneFixture()
    {
        services = new ScriptSceneServices(&host);
        services->setSceneProvider([this] { return document.scene(); });
        services->setClearDocument([this](QString* error) {
            return document.clear(error);
        });
        services->setInsertNode([this](SoNode* node, QString* error) {
            return document.insert(node, error);
        });
        registerScriptSceneServices(host.engine(), services);
        const auto facade = host.engine()->globalObject().property("__tonatiuhppSceneApi");
        host.engine()->globalObject().setProperty("tn", facade);
        host.engine()->globalObject().setProperty("tonatiuh", facade);
    }

    int childCount() const
    {
        if (!document.scene() || !document.scene()->getLayout())
            return -1;
        auto* group = dynamic_cast<SoGroup*>(
            document.scene()->getLayout()->getPart("group", true));
        return group ? group->getNumChildren() : -1;
    }
};
}

TEST(ScientificScriptSceneDocument, CreatesInsertsAndClearsInMemoryScene)
{
    TonatiuhCore::initializeCoin();
    CorePluginRegistry factories;
    ScriptSceneFixture fixture;

    ASSERT_EQ(fixture.childCount(), 0);
    const QJSValue run = fixture.host.evaluate(R"JS(
var root = new NodeObject;
root.setName('M3_2_Insert');
root.createNode('nested');
if (tn.InsertScene(root) !== undefined) throw new Error('Unexpected insert result');
if (!tn.getScene().getPart('world.sun.position')) throw new Error('Missing sun');
true;
)JS", "script-insert.tnhpps");
    ASSERT_FALSE(run.isError()) << run.toString().toStdString();
    ASSERT_TRUE(run.toBool());
    ASSERT_EQ(fixture.childCount(), 1);

    const QJSValue reset = fixture.host.evaluate(R"JS(
var oldScene = tn.getScene();
if (tn.Clear() !== undefined) throw new Error('Unexpected clear result');
if (!tn.getScene().getPart('world.sun.position')) throw new Error('No new scene');
// Coin wrappers pin old scene nodes so they cannot dangle after Clear.
if (!oldScene.getPart('world.sun.position')) throw new Error('Dangling old scene');
true;
)JS", "script-clear.tnhpps");
    ASSERT_FALSE(reset.isError()) << reset.toString().toStdString();
    EXPECT_TRUE(reset.toBool());
    EXPECT_EQ(fixture.childCount(), 0);

    const QJSValue cycle = fixture.host.evaluate(
        "tn.InsertScene(tn.getScene());", "invalid-cycle.tnhpps");
    ASSERT_TRUE(cycle.isError());
    EXPECT_TRUE(cycle.toString().contains("cannot insert the active scene"));
    EXPECT_EQ(fixture.childCount(), 0);
}

TEST(ScientificScriptSceneDocument, IndependentDocumentsAcrossHosts)
{
    TonatiuhCore::initializeCoin();
    CorePluginRegistry factories;
    ScriptSceneFixture first;
    ScriptSceneFixture second;

    const QJSValue firstResult = first.host.evaluate(
        "var node = new NodeObject; node.setName('first'); tn.InsertScene(node);",
        "host-first.tnhpps");
    ASSERT_FALSE(firstResult.isError()) << firstResult.toString().toStdString();
    EXPECT_EQ(first.childCount(), 1);
    EXPECT_EQ(second.childCount(), 0);

    const QJSValue secondResult = second.host.evaluate(
        "var node = new NodeObject; node.setName('second'); tn.InsertScene(node);",
        "host-second.tnhpps");
    ASSERT_FALSE(secondResult.isError()) << secondResult.toString().toStdString();
    EXPECT_EQ(first.childCount(), 1);
    EXPECT_EQ(second.childCount(), 1);

    const QJSValue clearFirst = first.host.evaluate("tn.Clear();", "host-first.tnhpps");
    ASSERT_FALSE(clearFirst.isError()) << clearFirst.toString().toStdString();
    EXPECT_EQ(first.childCount(), 0);
    EXPECT_EQ(second.childCount(), 1);
}

TEST(ScientificScriptSceneDocument, BadInsertionReportsSourceLine)
{
    TonatiuhCore::initializeCoin();
    CorePluginRegistry factories;
    ScriptSceneFixture fixture;

    const QJSValue result = fixture.host.evaluate(
        "tn.Clear();\n"
        "tn.InsertScene({notANode: true});\n", "invalid-insert.tnhpps");
    ASSERT_TRUE(result.isError());
    EXPECT_EQ(result.property("lineNumber").toInt(), 2);
    EXPECT_TRUE(result.toString().contains("requires a NodeObject"))
        << result.toString().toStdString();
    EXPECT_EQ(fixture.childCount(), 0);

    const QJSValue cycle = fixture.host.evaluate(
        "tn.InsertScene(tn.getScene());", "invalid-cycle.tnhpps");
    ASSERT_TRUE(cycle.isError());
    EXPECT_TRUE(cycle.toString().contains("cannot insert the active scene"));
    EXPECT_EQ(fixture.childCount(), 0);
}


TEST(ScientificScriptSceneDocument, ForwardingKeepsNativeErrorAtUserCallSite)
{
    TonatiuhCore::initializeCoin();
    ScriptSceneFixture fixture;

    // Match the headless bridge: preserve the QObject methods as functions,
    // rather than introduce JavaScript wrappers for each invocation.
    const QJSValue headlessBridge = fixture.host.evaluate(R"JS(
(function(global) {
  var scene = global.__tonatiuhppSceneApi;
  global.tn = {
    getScene: scene.getScene,
    Clear: scene.Clear,
    InsertScene: scene.InsertScene
  };
})(this);
)JS", "<headless-bootstrap-test>");
    ASSERT_FALSE(headlessBridge.isError()) << headlessBridge.toString().toStdString();

    const QJSValue headlessError = fixture.host.evaluate(
        "tn.Clear();\n"
        "tn.InsertScene({invalid: true});\n", "headless-forwarded.tnhpps");
    ASSERT_TRUE(headlessError.isError());
    EXPECT_EQ(headlessError.property("lineNumber").toInt(), 2);
    EXPECT_TRUE(headlessError.property("fileName").toString().contains("headless-forwarded.tnhpps"));
    EXPECT_TRUE(headlessError.toString().contains("requires a NodeObject"));

    // Match the GUI Proxy forwarding the same bound native methods.
    const QJSValue guiBridge = fixture.host.evaluate(R"JS(
(function(global) {
  var scene = global.__tonatiuhppSceneApi;
  var legacy = global.tn;
  global.tn = new Proxy(legacy, {
    get: function(target, property) {
      if (property === 'getScene') return scene.getScene;
      if (property === 'Clear') return scene.Clear;
      if (property === 'InsertScene') return scene.InsertScene;
      return target[property];
    }
  });
})(this);
)JS", "<gui-scene-api-test>");
    ASSERT_FALSE(guiBridge.isError()) << guiBridge.toString().toStdString();

    const QJSValue guiError = fixture.host.evaluate(
        "tn.Clear();\n"
        "tn.InsertScene({invalid: true});\n", "gui-forwarded.tnhpps");
    ASSERT_TRUE(guiError.isError());
    EXPECT_EQ(guiError.property("lineNumber").toInt(), 2);
    EXPECT_TRUE(guiError.property("fileName").toString().contains("gui-forwarded.tnhpps"));
    EXPECT_TRUE(guiError.toString().contains("requires a NodeObject"));
}
