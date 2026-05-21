#pragma once

#include <QString>
#include <QStringList>

namespace TonatiuhCore
{
void initializeCoin();
void initializeCoreTypes();
QStringList pluginSearchPaths(const QString& applicationDirPath);
void setProjectSearchPaths(const QString& fileName);
}
