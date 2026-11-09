#pragma once

#include <cmath>

#include "libraries/TonatiuhLibraries.h"
#include <QFile>
#include <QFileDevice>
#include <QSaveFile>
#include <QString>
#include <QTextStream>

#include "libraries/math/2D/Box2D.h"
#include "libraries/math/2D/Matrix2D.h"

// Widget-free legacy FluxAnalysis TSV formatting and atomic I/O reporting.
// The calculation of photon counts, power and projected cell area stays in
// FluxAnalysis; this helper must not change the accepted data layout.
namespace FluxAnalysisTsvWriter
{
inline bool fail(QString* error, const QString& message)
{
    if (error)
        *error = message;
    return false;
}

inline bool write(const QString& fileName, const Matrix2D<int>& bins,
                  const Box2D& box, double coefficient, bool withCoords,
                  QString* error = nullptr)
{
    if (error)
        error->clear();
    if (fileName.trimmed().isEmpty())
        return fail(error, "Flux analysis output path is empty.");
    if (bins.rows() <= 0 || bins.cols() <= 0 || bins.isEmpty() || !box.isValid()
        || !std::isfinite(coefficient))
        return fail(error, "Flux analysis output has invalid cells or normalization.");

    const double uStep = box.size().x / bins.rows();
    const double vStep = box.size().y / bins.cols();
    if (!std::isfinite(uStep) || !std::isfinite(vStep))
        return fail(error, "Flux analysis output has invalid cell coordinates.");

    // QSaveFile writes beside the destination then commits on success.
    // Legacy seven-argument callers may replace a previous TSV; write/flush
    // failures do not destroy an existing scientifically accepted output.
    QSaveFile file(fileName);
    if (!file.open(QIODevice::WriteOnly))
        return fail(error, QString("Cannot open flux output '%1': %2")
                          .arg(fileName, file.errorString()));

    QTextStream out(&file);
    if (withCoords) {
        out << "x(m)\ty(m)\tFlux(W/m2)\n";
        for (int r = 0; r < bins.rows(); ++r) {
            for (int c = 0; c < bins.cols(); ++c)
                out << box.min().x + uStep * (r + 0.5) << "\t"
                    << box.min().y + vStep * (c + 0.5) << "\t"
                    << bins(r, c) * coefficient << "\n";
        }
    } else {
        for (int r = 0; r < bins.rows(); ++r) {
            for (int c = 0; c < bins.cols(); ++c)
                out << bins(r, c) * coefficient << "\t";
            out << "\n";
        }
    }

    out.flush();
    const bool streamOk = out.status() == QTextStream::Ok;
    const bool flushed = file.flush();
    const QString fileError = file.errorString();
    const bool fileOk = file.error() == QFileDevice::NoError;
    if (!streamOk || !flushed || !fileOk) {
        file.cancelWriting();
        return fail(error, QString("Cannot finish flux output '%1': %2")
                          .arg(fileName, fileError));
    }
    if (!file.commit())
        return fail(error, QString("Cannot finish flux output '%1': %2")
                          .arg(fileName, file.errorString()));
    return true;
}
}
