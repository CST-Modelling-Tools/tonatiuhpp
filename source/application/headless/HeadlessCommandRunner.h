#pragma once

#include <QString>
#include <QStringList>
#include <qglobal.h>

class CorePluginRegistry;

class HeadlessCommandRunner
{
public:
    int run(const QStringList& arguments) const;

private:
    struct TraceSceneArguments
    {
        QString sceneFileName;
        ulong rays = 0;
        ulong seed = 0;
        bool hasRays = false;
        bool hasSeed = false;
        bool noExport = false;
    };

    int validateScene(const QString& fileName) const;
    int traceScene(const QStringList& args) const;
    int benchmark(const QStringList& args) const;
    void initializeSceneServices(const QString& fileName, CorePluginRegistry* plugins) const;
    bool parseTraceSceneArguments(const QStringList& args, TraceSceneArguments* parsed, QString* errorMessage) const;
    bool parseUnsignedLongOption(const QString& optionName, const QString& value, bool allowZero, ulong* parsed, QString* errorMessage) const;
    void printUsage() const;
    int printUsageError(const QString& message) const;
};
