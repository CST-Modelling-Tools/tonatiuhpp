#include "TonatiuhDiagnostics.h"
#include <QDebug>
#include <atomic>

namespace {
std::atomic<TonatiuhWarningPresenter> s_warningPresenter{nullptr};
}

void tonatiuhSetWarningPresenter(TonatiuhWarningPresenter presenter)
{
    s_warningPresenter.store(presenter);
}

void tonatiuhReportWarning(const QString& title, const QString& message)
{
    auto presenter = s_warningPresenter.load();
    if (presenter) presenter(title, message);
    else qWarning().noquote() << title << ":" << message;
}
