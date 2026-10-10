#include "NativeTraceSignature.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <tuple>

#include <QByteArray>
#include <QCryptographicHash>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>

#include "kernel/run/InstanceNode.h"
#include "kernel/run/RayTracer.h"

namespace
{
bool fail(QString* error, const QString& message)
{
    if (error)
        *error = message;
    return false;
}

QByteArray hexBits(std::uint64_t bits)
{
    return QByteArray::number(bits, 16).rightJustified(16, '0');
}

QString doubleBits(double value)
{
    return QString::fromLatin1(hexBits(std::bit_cast<std::uint64_t>(value)));
}
}

bool NativeTraceSignature::Event::operator<(const Event& other) const
{
    return std::tie(surfaceUrl, isFront, positionBits)
        < std::tie(other.surfaceUrl, other.isFront, other.positionBits);
}

QString NativeTraceSignature::outputPathFromEnvironment()
{
    return qEnvironmentVariable("TONATIUHPP_A0_TRACE_SIGNATURE_FILE");
}

bool NativeTraceSignature::validateRayCount(ulong rays, QString* errorMessage)
{
    if (rays == 0 || rays > kMaxDiagnosticRays)
        return fail(errorMessage, QStringLiteral(
            "A0 signature diagnostics require between 1 and %1 rays; "
            "unset TONATIUHPP_A0_TRACE_SIGNATURE_FILE for normal large traces.")
            .arg(kMaxDiagnosticRays));
    return true;
}

void NativeTraceSignature::add(const RayTracerHit& hit)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!std::isfinite(hit.position.x) || !std::isfinite(hit.position.y)
        || !std::isfinite(hit.position.z)) {
        ++m_nonfiniteCount;
        return;
    }

    m_events.push_back(Event{
        hit.surface ? hit.surface->getURL() : QString(),
        hit.isFront,
        {std::bit_cast<std::uint64_t>(hit.position.x),
         std::bit_cast<std::uint64_t>(hit.position.y),
         std::bit_cast<std::uint64_t>(hit.position.z)}
    });
}

bool NativeTraceSignature::write(const QString& fileName, const QString& mode,
                                 const Inputs& inputs, QString* errorMessage) const
{
    if (!validateRayCount(inputs.rays, errorMessage))
        return false;
    if (fileName.isEmpty() || inputs.sceneFile.isEmpty())
        return fail(errorMessage, "The diagnostic output and scene file paths must be set.");

    QFile sceneFile(inputs.sceneFile);
    if (!sceneFile.open(QIODevice::ReadOnly))
        return fail(errorMessage, QString("Cannot hash scene file %1: %2")
            .arg(inputs.sceneFile, sceneFile.errorString()));
    QCryptographicHash sceneHash(QCryptographicHash::Sha256);
    if (!sceneHash.addData(&sceneFile))
        return fail(errorMessage, "Could not hash scene file bytes.");
    sceneFile.close();

    std::vector<Event> events;
    std::uint64_t nonfinite = 0;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        events = m_events;
        nonfinite = m_nonfiniteCount;
    }
    if (nonfinite != 0)
        return fail(errorMessage, "Non-finite hit coordinates make this trace incomparable.");
    if (events.empty())
        return fail(errorMessage, "No scientific hit events were recorded; cannot prove equivalence.");

    std::sort(events.begin(), events.end());
    QCryptographicHash hash(QCryptographicHash::Sha256);
    std::uint64_t frontCount = 0;
    for (const Event& event : events) {
        if (event.isFront)
            ++frontCount;
        const QByteArray url = event.surfaceUrl.toUtf8();
        // Explicit length and fixed-width binary64 hex protect delimiters,
        // preserve -0 and avoid JSON number precision loss.
        hash.addData(QByteArray::number(url.size()));
        hash.addData(":");
        hash.addData(url);
        hash.addData(event.isFront ? "|1|" : "|0|");
        for (std::uint64_t bits : event.positionBits) {
            hash.addData(hexBits(bits));
            hash.addData("|");
        }
        hash.addData("\n");
    }

    QJsonObject result;
    result.insert("schema", "tonatiuhpp.a0.native-hits.v1");
    result.insert("mode", mode);
    result.insert("scene_sha256", QString::fromLatin1(sceneHash.result().toHex()));
    result.insert("rays", QString::number(static_cast<qulonglong>(inputs.rays)));
    result.insert("seed", QString::number(static_cast<qulonglong>(inputs.seed)));
    result.insert("grid_width", inputs.sunGridWidth);
    result.insert("grid_height", inputs.sunGridHeight);
    result.insert("aperture_area_bits", doubleBits(inputs.apertureArea));
    result.insert("irradiance_bits", doubleBits(inputs.irradiance));
    result.insert("power_per_ray_bits", doubleBits(inputs.powerPerRay));
    result.insert("hit_count", QString::number(static_cast<qulonglong>(events.size())));
    result.insert("front_hit_count", QString::number(static_cast<qulonglong>(frontCount)));
    result.insert("hit_sha256", QString::fromLatin1(hash.result().toHex()));

    QSaveFile output(fileName);
    if (!output.open(QIODevice::WriteOnly))
        return fail(errorMessage, QString("Cannot create signature file %1: %2")
            .arg(fileName, output.errorString()));
    const QByteArray json = QJsonDocument(result).toJson(QJsonDocument::Indented);
    if (output.write(json) != json.size() || !output.commit())
        return fail(errorMessage, QString("Could not write signature file %1: %2")
            .arg(fileName, output.errorString()));
    return true;
}
