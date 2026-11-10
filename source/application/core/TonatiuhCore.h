#pragma once
#include <QJsonObject>

#include <QString>
#include <QStringList>

namespace TonatiuhCore
{
void initializeCoin();
void initializeCoreTypes();
QStringList pluginSearchPaths(const QString& applicationDirPath);
void setProjectSearchPaths(const QString& fileName);
}

// Headless execution accepts only plugins explicitly built for the shared
// nonvisual ABI. Existing GUI plugins remain available in GUI mode.
namespace TonatiuhCore {
    void setHeadlessPluginMode(bool enabled);
    bool headlessPluginMode();
    bool allowsPluginMetadata(const QJsonObject& metadata);
}
