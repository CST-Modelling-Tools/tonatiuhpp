#pragma once

class MainWindow;

// Included only in BUILD_TESTING GUI builds. An explicit test environment
// variable enables the isolated, bounded native interactive acceptance.
void installM5GuiNativeAcceptance(MainWindow* mainWindow);
