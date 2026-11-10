#pragma once
#include <QtCore/qglobal.h>

// All QWidget/QCustomPlot/PhotonsWidget exported C++ symbols live in this
// optional GUI adapter, never in TonatiuhKernel or TonatiuhLibraries.
#if defined(TONATIUH_GUI_PRESENTATION_EXPORT)
#  define TONATIUH_GUI_PRESENTATION Q_DECL_EXPORT
#else
#  define TONATIUH_GUI_PRESENTATION Q_DECL_IMPORT
#endif
