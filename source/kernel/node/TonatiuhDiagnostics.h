#pragma once
#include "kernel/TonatiuhKernel.h"
#include <QString>

using TonatiuhWarningPresenter = void (*)(const QString&, const QString&);
TONATIUH_KERNEL void tonatiuhSetWarningPresenter(TonatiuhWarningPresenter);
TONATIUH_KERNEL void tonatiuhReportWarning(const QString& title, const QString& message);
