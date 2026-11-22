#pragma once

#include <QJsonObject>
#include <QString>

class MainWindow;

// Test-only real GUI edit commands and QUndoStack regression. BUILD_TESTING only.
bool runNativeSceneUndoRegression(MainWindow* window, QJsonObject* evidence,
                                  QString* error);
