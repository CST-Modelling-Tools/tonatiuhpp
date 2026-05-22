#pragma once

#include <QString>

class TSceneKit;

class BenchmarkRunner
{
public:
    QString sceneFileName(const QString& configFileName, QString* errorMessage) const;
    int run(const QString& configFileName, TSceneKit* scene, QString* errorMessage) const;
};
