#include "NativeRayHistorySignature.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdint>

#include <QByteArray>
#include <QCryptographicHash>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>

#include "kernel/run/InstanceNode.h"

namespace
{
bool fail(QString* error, const QString& message)
{
    if (error)
        *error = message;
    return false;
}

QByteArray bits(double value)
{
    return QByteArray::number(std::bit_cast<std::uint64_t>(value), 16)
        .rightJustified(16, '0');
}

bool finite(const vec3d& v)
{
    return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
}

void appendInteger(QCryptographicHash& hash, std::uint64_t value)
{
    hash.addData(QByteArray::number(value));
    hash.addData("|");
}

void appendVector(QCryptographicHash& hash, const vec3d& v)
{
    hash.addData(bits(v.x));
    hash.addData("|");
    hash.addData(bits(v.y));
    hash.addData("|");
    hash.addData(bits(v.z));
    hash.addData("|");
}

void appendSurface(QCryptographicHash& hash, const InstanceNode* surface)
{
    const QByteArray url = surface ? surface->getURL().toUtf8() : QByteArray();
    appendInteger(hash, static_cast<std::uint64_t>(url.size()));
    hash.addData(url);
    hash.addData("|");
}
} // namespace

QString NativeRayHistorySignature::outputPathFromEnvironment()
{
    return qEnvironmentVariable("TONATIUHPP_A0_RAY_HISTORY_FILE");
}

void NativeRayHistorySignature::add(const RayTracerHistory& history)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_histories.push_back(history);
}

bool NativeRayHistorySignature::write(const QString& fileName, const QString& mode,
                                      const NativeTraceSignature::Inputs& inputs,
                                      QString* errorMessage) const
{
    if (!NativeTraceSignature::validateRayCount(inputs.rays, errorMessage))
        return false;
    if (fileName.isEmpty() || inputs.sceneFile.isEmpty())
        return fail(errorMessage, "The history output and original scene paths are required.");
    if (mode != QStringLiteral("native-gui") && mode != QStringLiteral("headless-cli"))
        return fail(errorMessage, "Unsupported native ray-history capture mode.");

    QFile sceneFile(inputs.sceneFile);
    if (!sceneFile.open(QIODevice::ReadOnly))
        return fail(errorMessage, QString("Cannot open scene for ray-history identity: %1")
            .arg(sceneFile.errorString()));
    QCryptographicHash sceneHash(QCryptographicHash::Sha256);
    if (!sceneHash.addData(&sceneFile))
        return fail(errorMessage, "Cannot hash the original scene bytes.");

    std::vector<RayTracerHistory> histories;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        histories = m_histories;
    }
    if (histories.size() != static_cast<std::size_t>(inputs.rays))
        return fail(errorMessage, QString("Incomplete ray histories: captured %1 of %2 rays.")
            .arg(static_cast<qulonglong>(histories.size()))
            .arg(static_cast<qulonglong>(inputs.rays)));

    std::sort(histories.begin(), histories.end(),
              [](const RayTracerHistory& a, const RayTracerHistory& b) {
                  return a.rayIndex < b.rayIndex;
              });

    QCryptographicHash hash(QCryptographicHash::Sha256);
    std::uint64_t interactionCount = 0;
    std::uint64_t continuedCount = 0;
    std::uint64_t surfaceCount = 0;
    std::uint64_t airCount = 0;
    std::uint64_t escapeCount = 0;
    std::uint64_t maxDepth = 0;
    for (std::size_t i = 0; i < histories.size(); ++i) {
        const RayTracerHistory& ray = histories[i];
        if (ray.rayIndex != static_cast<std::uint64_t>(i))
            return fail(errorMessage, "Missing or duplicated stable ray index.");
        if (!finite(ray.sunOrigin) || !finite(ray.sunDirection)
            || !finite(ray.terminalOrigin) || !finite(ray.terminalDirection)
            || std::isnan(ray.terminalDistance))
            return fail(errorMessage, "Invalid ray geometry or terminal distance.");
        if (ray.termination == RayTracerHistoryTermination::Surface) {
            if (ray.steps.empty() || ray.steps.back().continued)
                return fail(errorMessage, "Surface termination has no terminal interaction.");
            ++surfaceCount;
        } else if (ray.termination == RayTracerHistoryTermination::Air) {
            ++airCount;
        } else if (ray.termination == RayTracerHistoryTermination::Escaped) {
            ++escapeCount;
        } else {
            return fail(errorMessage, "Unknown ray termination reason.");
        }

        hash.addData("R|");
        appendInteger(hash, ray.rayIndex);
        appendVector(hash, ray.sunOrigin);
        appendVector(hash, ray.sunDirection);
        appendInteger(hash, static_cast<std::uint64_t>(ray.steps.size()));
        maxDepth = std::max(maxDepth, static_cast<std::uint64_t>(ray.steps.size()));
        for (std::size_t j = 0; j < ray.steps.size(); ++j) {
            const RayTracerHistoryStep& step = ray.steps[j];
            if (!step.surface || !finite(step.position)
                || !finite(step.incomingOrigin) || !finite(step.incomingDirection)
                || !finite(step.outgoingOrigin) || !finite(step.outgoingDirection)
                || (!step.continued && j + 1 != ray.steps.size())
                || (!step.continued && ray.termination != RayTracerHistoryTermination::Surface))
                return fail(errorMessage, "Invalid or nonsequential ray interaction.");
            ++interactionCount;
            if (step.continued)
                ++continuedCount;
            hash.addData("S|");
            appendSurface(hash, step.surface);
            appendInteger(hash, step.isFront ? 1 : 0);
            appendInteger(hash, step.continued ? 1 : 0);
            appendVector(hash, step.position);
            appendVector(hash, step.incomingOrigin);
            appendVector(hash, step.incomingDirection);
            appendVector(hash, step.outgoingOrigin);
            appendVector(hash, step.outgoingDirection);
        }
        hash.addData("T|");
        appendInteger(hash, static_cast<std::uint64_t>(ray.termination));
        appendVector(hash, ray.terminalOrigin);
        appendVector(hash, ray.terminalDirection);
        hash.addData(bits(ray.terminalDistance));
        hash.addData("\n");
    }

    QJsonObject json;
    json.insert("schema", "tonatiuhpp.a0.native-ray-histories.v1");
    json.insert("mode", mode);
    json.insert("scene_sha256", QString::fromLatin1(sceneHash.result().toHex()));
    json.insert("rays", QString::number(static_cast<qulonglong>(inputs.rays)));
    json.insert("seed", QString::number(static_cast<qulonglong>(inputs.seed)));
    json.insert("grid_width", inputs.sunGridWidth);
    json.insert("grid_height", inputs.sunGridHeight);
    json.insert("aperture_area_bits", QString::fromLatin1(bits(inputs.apertureArea)));
    json.insert("irradiance_bits", QString::fromLatin1(bits(inputs.irradiance)));
    json.insert("power_per_ray_bits", QString::fromLatin1(bits(inputs.powerPerRay)));
    json.insert("ray_count", QString::number(static_cast<qulonglong>(histories.size())));
    json.insert("interaction_count", QString::number(static_cast<qulonglong>(interactionCount)));
    json.insert("continued_count", QString::number(static_cast<qulonglong>(continuedCount)));
    json.insert("surface_terminations", QString::number(static_cast<qulonglong>(surfaceCount)));
    json.insert("air_terminations", QString::number(static_cast<qulonglong>(airCount)));
    json.insert("escaped_rays", QString::number(static_cast<qulonglong>(escapeCount)));
    json.insert("max_depth", QString::number(static_cast<qulonglong>(maxDepth)));
    json.insert("history_sha256", QString::fromLatin1(hash.result().toHex()));

    QSaveFile output(fileName);
    if (!output.open(QIODevice::WriteOnly))
        return fail(errorMessage, QString("Cannot create ray-history file %1: %2")
            .arg(fileName, output.errorString()));
    const QByteArray bytes = QJsonDocument(json).toJson(QJsonDocument::Indented);
    if (output.write(bytes) != bytes.size() || !output.commit())
        return fail(errorMessage, QString("Cannot finalize ray-history file %1: %2")
            .arg(fileName, output.errorString()));
    return true;
}
