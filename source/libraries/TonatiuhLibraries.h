#pragma once

#include <QtCore/qglobal.h>

#if defined(TONATIUH_LIBRARIES_EXPORT)
    #define TONATIUH_LIBRARIES Q_DECL_EXPORT
#else
    #define TONATIUH_LIBRARIES Q_DECL_IMPORT
#endif

// QCustomPlot is now supplied exclusively by the presentation DLL.
#if defined(TONATIUH_GUI_PRESENTATION_EXPORT)
    #define QCUSTOMPLOT_COMPILE_LIBRARY
#else
    #define QCUSTOMPLOT_USE_LIBRARY
#endif
