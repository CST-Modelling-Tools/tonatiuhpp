#pragma once

// M3 ownership extraction: one File-run owner per ScriptHost.
// The buffer can hold Photon.surface pointers into the instance tree.
// Destroy in dependency order: buffer, exporter, plugin, instance tree,
// then release the strong Coin scene reference. Do not reorder.

#include <cstdint>
#include <memory>

#include <QByteArray>
#include <QPluginLoader>

#include "core/SceneInstanceBuilder.h"
#include "kernel/photons/PhotonsAbstract.h"
#include "kernel/photons/PhotonsBuffer.h"
#include "kernel/photons/PhotonsSettings.h"
#include "kernel/scene/TSceneKit.h"
#include "script/ScriptPhotonExportConfig.h"

struct HeadlessScriptFileExportResources final
{
    explicit HeadlessScriptFileExportResources(TSceneKit* source)
        : scene(source)
    {
        // Scripts may clear/load a new scene before this owner is destroyed.
        if (scene) scene->ref();
    }

    ~HeadlessScriptFileExportResources()
    {
        // Explicit dependency ordering overrides member declaration order.
        buffer.reset();
        exporter.reset();
        plugin.reset();
        tree.sceneRoot.reset();
        if (scene) scene->unref();
    }

    HeadlessScriptFileExportResources(const HeadlessScriptFileExportResources&) = delete;
    HeadlessScriptFileExportResources& operator=(const HeadlessScriptFileExportResources&) = delete;

    TSceneKit* scene = nullptr;
    SceneInstanceTree tree;
    std::unique_ptr<QPluginLoader> plugin;
    std::unique_ptr<PhotonsAbstract> exporter;
    std::unique_ptr<PhotonsBuffer> buffer;
    PhotonsSettings settings{};
    ScriptPhotonExportState options;
    std::uint64_t completedRays = 0;
    bool poisoned = false;
    bool appendable = true;
    QByteArray outputDigest;
    QByteArray parametersDigest;
};
