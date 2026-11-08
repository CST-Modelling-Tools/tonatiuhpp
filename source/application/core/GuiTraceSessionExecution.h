#pragma once

#include <QFuture>
#include <QString>

#include "core/SimulationSession.h"

// Widget-free GUI session execution boundary. The caller supplies a synchronous
// progress adapter (normally a QFutureWatcher plus a modal progress dialog).
// The session remains the sole owner of worker start/join and result state.
namespace GuiTraceSessionExecution
{
struct Outcome
{
    bool started = false;
    bool completed = false;
    bool cancelled = false;
    bool exportFailed = false;
    ulong raysTraced = 0;
    QString error;
};

template <typename ProgressAdapter>
Outcome runPrepared(SimulationSession& session, ProgressAdapter&& progressAdapter)
{
    Outcome outcome;
    if (!session.start(&outcome.error))
        return outcome;

    outcome.started = true;
    // The GUI watcher may cancel a copy of this shared QFuture. Do not join
    // until the progress adapter has returned (e.g. after dialog.exec()).
    const QFuture<void> future = session.future();
    progressAdapter(future);
    outcome.completed = session.waitForFinished(&outcome.error);
    outcome.cancelled = session.state() == SimulationSession::State::Cancelled;
    outcome.exportFailed = session.exportFailed();
    outcome.raysTraced = session.result().raysTraced;
    return outcome;
}
} // namespace GuiTraceSessionExecution
