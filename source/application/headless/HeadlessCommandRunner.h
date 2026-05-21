#pragma once

#include <QStringList>

class HeadlessCommandRunner
{
public:
    int run(const QStringList& arguments) const;

private:
    int validateScene(const QString& fileName) const;
    void printUsage() const;
    int printUsageError(const QString& message) const;
};
