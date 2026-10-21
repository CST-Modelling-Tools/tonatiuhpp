#pragma once

class MainWindow;
class ScriptHost;

// Only this GUI adapter may couple the script scene bridge to MainWindow.
// Domain NodeObject and ScriptSceneServices must remain widget-independent.
void installGuiScriptSceneAdapter(ScriptHost* host, MainWindow* window);
