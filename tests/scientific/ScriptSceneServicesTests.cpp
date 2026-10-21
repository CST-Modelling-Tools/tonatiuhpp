#include <gtest/gtest.h>

#include <QJSValue>

#include "core/CorePluginRegistry.h"
#include "core/TonatiuhCore.h"
#include "script/ScriptHost.h"
#include "script/ScriptSceneServices.h"

TEST(ScientificScriptSceneServices, FactoriesArePerHostWithoutGlobalPointers)
{
    TonatiuhCore::initializeCoin();
    CorePluginRegistry factories;

    ScriptHost first;
    ScriptHost second;
    auto* firstContext = new ScriptSceneServices(&first);
    auto* secondContext = new ScriptSceneServices(&second);
    firstContext->setNodeCreator([&factories](ScriptSceneServices::FactoryKind kind,
                                               const QString& name) -> SoNode* {
        if (kind != ScriptSceneServices::FactoryKind::Shape)
            return nullptr;
        return factories.createSceneNode("shape", name);
    });
    registerScriptSceneServices(first.engine(), firstContext);
    registerScriptSceneServices(second.engine(), secondContext);

    const QJSValue firstResult = first.evaluate(
        "var root = new NodeObject;"
        "var shape = root.createShape();"
        "var created = shape.insertSurface('Planar');"
        "created !== undefined && created !== null;", "first.tnhpps");
    ASSERT_FALSE(firstResult.isError()) << firstResult.toString().toStdString();
    EXPECT_TRUE(firstResult.toBool());

    const QJSValue secondError = second.evaluate(
        "var root = new NodeObject;"
        "root.createShape().insertSurface('Planar');", "second.tnhpps");
    ASSERT_TRUE(secondError.isError());
    EXPECT_TRUE(secondError.toString().contains("unknown shape"))
        << secondError.toString().toStdString();

    const QJSValue after = first.evaluate(
        "shape.insertSurface('Planar') !== null;", "first.tnhpps");
    ASSERT_FALSE(after.isError()) << after.toString().toStdString();
    EXPECT_TRUE(after.toBool());
}

TEST(ScientificScriptSceneServices, UnknownFactoriesProduceScriptErrors)
{
    TonatiuhCore::initializeCoin();
    CorePluginRegistry factories;
    ScriptHost host;
    auto* context = new ScriptSceneServices(&host);
    context->setNodeCreator([&factories](ScriptSceneServices::FactoryKind kind,
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
    registerScriptSceneServices(host.engine(), context);

    const QJSValue result = host.evaluate(
        "var root = new NodeObject;\n"
        "root.createShape().insertMaterial('missing-m3-2-factory');\n", "missing-factory.tnhpps");
    ASSERT_TRUE(result.isError());
    EXPECT_EQ(result.property("lineNumber").toInt(), 2);
    EXPECT_TRUE(result.toString().contains("unknown material"));
}
