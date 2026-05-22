#include "BenchmarkRunner.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <vector>

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QSaveFile>
#include <QTextStream>
#include <QThread>
#include <QtEndian>

#include "core/RayTraceRunner.h"
#include "kernel/run/RayTracer.h"
#include "libraries/math/gcf.h"

namespace
{
constexpr double kBenchmarkV1ReceiverZ = 80.;
constexpr double kBenchmarkV1TiltDegrees = 210.515 - 180.;
constexpr double kMegawatt = 1.e6;
constexpr size_t kMaxGridCells = 10000000;

struct Bounds
{
    double xMin = -2.;
    double xMax = 2.;
    double yMin = -2.;
    double yMax = 2.;
};

struct Grid
{
    int width = 100;
    int height = 100;
};

struct BenchmarkConfig
{
    QString benchmark = "benchmark_v1";
    QString sceneFile = "benchmark_v1.tnhpp";
    ulong rays = 500000000;
    ulong seed = 123456789;
    int targetSideId = 1;
    Bounds bounds;
    Grid grid;
    bool photonExport = false;
    QString outputFile = "benchmark_result.json";
    QString referenceFile;
};

struct ReferenceConfig
{
    bool enabled = false;
    double totalPowerMw = 0.;
    double maximumFluxMwM2 = 0.;
    QString fluxGridSha256;
    double totalPowerTolerancePercent = 0.1;
    double maximumFluxTolerancePercent = 5.;
};

struct BenchmarkMetrics
{
    double totalPowerMw = 0.;
    double minimumFluxMwM2 = 0.;
    double averageFluxMwM2 = 0.;
    double maximumFluxMwM2 = 0.;
    QString fluxGridSha256;
};

bool fail(QString* errorMessage, const QString& message)
{
    if (errorMessage)
        *errorMessage = message;
    return false;
}

QString resolveRelativePath(const QDir& baseDir, const QString& path)
{
    QFileInfo info(path);
    if (info.isAbsolute())
        return info.absoluteFilePath();
    return QFileInfo(baseDir.absoluteFilePath(path)).absoluteFilePath();
}

bool readJsonObject(const QString& fileName, QJsonObject* object, QString* errorMessage)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly))
        return fail(errorMessage, QString("Cannot open %1: %2").arg(fileName, file.errorString()));

    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError)
        return fail(errorMessage, QString("Cannot parse %1: %2").arg(fileName, parseError.errorString()));
    if (!document.isObject())
        return fail(errorMessage, QString("%1 must contain a JSON object.").arg(fileName));

    if (object)
        *object = document.object();
    return true;
}

bool parseFiniteDouble(const QJsonObject& object, const QString& name, double* value, QString* errorMessage)
{
    if (!object.contains(name))
        return true;
    if (!object.value(name).isDouble())
        return fail(errorMessage, QString("%1 must be a number.").arg(name));

    const double parsed = object.value(name).toDouble();
    if (!std::isfinite(parsed))
        return fail(errorMessage, QString("%1 must be finite.").arg(name));
    if (value)
        *value = parsed;
    return true;
}

bool parsePositiveInt(const QJsonObject& object, const QString& name, int* value, QString* errorMessage)
{
    if (!object.contains(name))
        return true;
    if (!object.value(name).isDouble())
        return fail(errorMessage, QString("%1 must be a positive integer.").arg(name));

    const double parsed = object.value(name).toDouble();
    if (!std::isfinite(parsed) || std::floor(parsed) != parsed || parsed <= 0 || parsed > std::numeric_limits<int>::max())
        return fail(errorMessage, QString("%1 must be a positive integer.").arg(name));
    if (value)
        *value = static_cast<int>(parsed);
    return true;
}

bool parseULong(const QJsonObject& object, const QString& name, bool positiveOnly, ulong* value, QString* errorMessage)
{
    if (!object.contains(name))
        return true;
    if (!object.value(name).isDouble())
        return fail(errorMessage, QString("%1 must be an integer.").arg(name));

    const double parsed = object.value(name).toDouble();
    if (!std::isfinite(parsed) || std::floor(parsed) != parsed)
        return fail(errorMessage, QString("%1 must be an integer.").arg(name));
    if (positiveOnly && parsed <= 0.)
        return fail(errorMessage, QString("%1 must be greater than zero.").arg(name));
    if (parsed < 0. || parsed > static_cast<double>(std::numeric_limits<ulong>::max()))
        return fail(errorMessage, QString("%1 is outside the supported range.").arg(name));
    if (value)
        *value = static_cast<ulong>(parsed);
    return true;
}

bool parseConfig(const QString& configFileName, BenchmarkConfig* config, QString* errorMessage)
{
    QJsonObject object;
    if (!readJsonObject(configFileName, &object, errorMessage))
        return false;

    BenchmarkConfig parsed;
    if (object.contains("benchmark")) {
        if (!object.value("benchmark").isString())
            return fail(errorMessage, "benchmark must be a string.");
        parsed.benchmark = object.value("benchmark").toString();
    }
    if (object.contains("scene_file")) {
        if (!object.value("scene_file").isString())
            return fail(errorMessage, "scene_file must be a string.");
        parsed.sceneFile = object.value("scene_file").toString();
    }
    if (parsed.sceneFile.trimmed().isEmpty())
        return fail(errorMessage, "scene_file must not be empty.");
    if (!parseULong(object, "rays", true, &parsed.rays, errorMessage))
        return false;
    if (!parseULong(object, "seed", false, &parsed.seed, errorMessage))
        return false;
    if (object.contains("target_side_id")) {
        if (!object.value("target_side_id").isDouble())
            return fail(errorMessage, "target_side_id must be 0 or 1.");
        const double side = object.value("target_side_id").toDouble();
        if (std::floor(side) != side || (side != 0. && side != 1.))
            return fail(errorMessage, "target_side_id must be 0 or 1.");
        parsed.targetSideId = static_cast<int>(side);
    }

    if (object.contains("target_bounds")) {
        if (!object.value("target_bounds").isObject())
            return fail(errorMessage, "target_bounds must be an object.");
        const QJsonObject bounds = object.value("target_bounds").toObject();
        if (!parseFiniteDouble(bounds, "x_min", &parsed.bounds.xMin, errorMessage) ||
            !parseFiniteDouble(bounds, "x_max", &parsed.bounds.xMax, errorMessage) ||
            !parseFiniteDouble(bounds, "y_min", &parsed.bounds.yMin, errorMessage) ||
            !parseFiniteDouble(bounds, "y_max", &parsed.bounds.yMax, errorMessage))
            return false;
    }
    if (parsed.bounds.xMax <= parsed.bounds.xMin || parsed.bounds.yMax <= parsed.bounds.yMin)
        return fail(errorMessage, "target_bounds must define positive extents.");

    if (object.contains("target_grid")) {
        if (!object.value("target_grid").isObject())
            return fail(errorMessage, "target_grid must be an object.");
        const QJsonObject grid = object.value("target_grid").toObject();
        if (!parsePositiveInt(grid, "width", &parsed.grid.width, errorMessage) ||
            !parsePositiveInt(grid, "height", &parsed.grid.height, errorMessage))
            return false;
    }
    const size_t gridWidth = static_cast<size_t>(parsed.grid.width);
    const size_t gridHeight = static_cast<size_t>(parsed.grid.height);
    if (gridWidth > std::numeric_limits<size_t>::max() / gridHeight)
        return fail(errorMessage, "target_grid dimensions are too large.");
    if (gridWidth * gridHeight > kMaxGridCells)
        return fail(errorMessage, QString("target_grid must not exceed %1 cells.").arg(static_cast<qulonglong>(kMaxGridCells)));

    if (object.contains("photon_export")) {
        if (!object.value("photon_export").isBool())
            return fail(errorMessage, "photon_export must be false.");
        parsed.photonExport = object.value("photon_export").toBool();
        if (parsed.photonExport)
            return fail(errorMessage, "benchmark headless mode does not support photon_export=true.");
    }
    if (object.contains("output_file")) {
        if (!object.value("output_file").isString())
            return fail(errorMessage, "output_file must be a string.");
        parsed.outputFile = object.value("output_file").toString();
    }
    if (parsed.outputFile.trimmed().isEmpty())
        return fail(errorMessage, "output_file must not be empty.");
    if (object.contains("reference_file")) {
        if (!object.value("reference_file").isString())
            return fail(errorMessage, "reference_file must be a string.");
        parsed.referenceFile = object.value("reference_file").toString();
    }

    if (config)
        *config = parsed;
    return true;
}

bool parseReference(const QString& referenceFileName, ReferenceConfig* reference, QString* errorMessage)
{
    if (referenceFileName.isEmpty())
        return true;

    QJsonObject object;
    if (!readJsonObject(referenceFileName, &object, errorMessage))
        return false;

    ReferenceConfig parsed;
    parsed.enabled = true;
    if (!object.contains("total_power_mw") || !object.contains("maximum_flux_mw_m2"))
        return fail(errorMessage, "reference must define total_power_mw and maximum_flux_mw_m2.");
    if (!parseFiniteDouble(object, "total_power_mw", &parsed.totalPowerMw, errorMessage) ||
        !parseFiniteDouble(object, "maximum_flux_mw_m2", &parsed.maximumFluxMwM2, errorMessage))
        return false;
    if (!object.value("flux_grid_sha256").isString())
        return fail(errorMessage, "reference flux_grid_sha256 must be a string.");
    parsed.fluxGridSha256 = object.value("flux_grid_sha256").toString();

    if (object.contains("tolerances")) {
        if (!object.value("tolerances").isObject())
            return fail(errorMessage, "reference tolerances must be an object.");
        const QJsonObject tolerances = object.value("tolerances").toObject();
        if (!parseFiniteDouble(tolerances, "total_power_relative_percent", &parsed.totalPowerTolerancePercent, errorMessage) ||
            !parseFiniteDouble(tolerances, "maximum_flux_relative_percent", &parsed.maximumFluxTolerancePercent, errorMessage))
            return false;
    }
    if (parsed.totalPowerTolerancePercent < 0. || parsed.maximumFluxTolerancePercent < 0.)
        return fail(errorMessage, "reference tolerances must be non-negative.");

    if (reference)
        *reference = parsed;
    return true;
}

QJsonObject boundsToJson(const Bounds& bounds)
{
    QJsonObject object;
    object["x_min"] = bounds.xMin;
    object["x_max"] = bounds.xMax;
    object["y_min"] = bounds.yMin;
    object["y_max"] = bounds.yMax;
    return object;
}

QJsonObject gridToJson(const Grid& grid)
{
    QJsonObject object;
    object["width"] = grid.width;
    object["height"] = grid.height;
    return object;
}

double relativeErrorPercent(double actual, double reference)
{
    if (reference == 0.)
        return actual == 0. ? 0. : 1.e300;
    return std::abs((actual - reference) / reference) * 100.;
}

QString sha256Float64LittleEndian(const std::vector<double>& values)
{
    QCryptographicHash hash(QCryptographicHash::Sha256);
    for (double value : values) {
        quint64 bits = 0;
        std::memcpy(&bits, &value, sizeof(value));
        bits = qToLittleEndian(bits);
        hash.addData(reinterpret_cast<const char*>(&bits), sizeof(bits));
    }
    return QString::fromLatin1(hash.result().toHex());
}

class BenchmarkAccumulator
{
public:
    explicit BenchmarkAccumulator(const BenchmarkConfig& config):
        m_config(config),
        m_hits(static_cast<size_t>(config.grid.width) * static_cast<size_t>(config.grid.height), 0),
        m_targetSideIsFront(config.targetSideId != 0),
        m_yScale(1. / std::cos(kBenchmarkV1TiltDegrees * gcf::degree)),
        m_xBinScale(config.grid.width / (config.bounds.xMax - config.bounds.xMin)),
        m_yBinScale(config.grid.height / (config.bounds.yMax - config.bounds.yMin))
    {
    }

    void onHit(const RayTracerHit& hit)
    {
        if (hit.isFront != m_targetSideIsFront)
            return;

        const double x = hit.position.x;
        const double y = (hit.position.z - kBenchmarkV1ReceiverZ) * m_yScale;
        if (!std::isfinite(x) || !std::isfinite(y))
            return;
        if (x < m_config.bounds.xMin || x > m_config.bounds.xMax ||
            y < m_config.bounds.yMin || y > m_config.bounds.yMax)
            return;

        int column = static_cast<int>(std::floor((x - m_config.bounds.xMin) * m_xBinScale));
        int row = static_cast<int>(std::floor((y - m_config.bounds.yMin) * m_yBinScale));
        if (column == m_config.grid.width)
            --column;
        if (row == m_config.grid.height)
            --row;
        if (column < 0 || column >= m_config.grid.width || row < 0 || row >= m_config.grid.height)
            return;

        ++m_hits[static_cast<size_t>(row) * static_cast<size_t>(m_config.grid.width) + static_cast<size_t>(column)];
        ++m_totalHits;
    }

    void merge(const BenchmarkAccumulator& other)
    {
        for (size_t index = 0; index < m_hits.size(); ++index)
            m_hits[index] += other.m_hits[index];
        m_totalHits += other.m_totalHits;
    }

    BenchmarkMetrics metrics(double powerPerRay) const
    {
        BenchmarkMetrics result;
        const double xStep = (m_config.bounds.xMax - m_config.bounds.xMin) / m_config.grid.width;
        const double yStep = (m_config.bounds.yMax - m_config.bounds.yMin) / m_config.grid.height;
        const double cellArea = xStep * yStep;

        std::vector<double> fluxGrid;
        fluxGrid.reserve(m_hits.size());

        result.minimumFluxMwM2 = std::numeric_limits<double>::max();
        for (qulonglong hits : m_hits) {
            const double flux = cellArea > 0. ? static_cast<double>(hits) * powerPerRay / cellArea / kMegawatt : 0.;
            fluxGrid.push_back(flux);
            result.minimumFluxMwM2 = std::min(result.minimumFluxMwM2, flux);
            result.maximumFluxMwM2 = std::max(result.maximumFluxMwM2, flux);
            result.averageFluxMwM2 += flux;
        }

        if (!fluxGrid.empty())
            result.averageFluxMwM2 /= static_cast<double>(fluxGrid.size());
        else
            result.minimumFluxMwM2 = 0.;

        result.totalPowerMw = static_cast<double>(m_totalHits) * powerPerRay / kMegawatt;
        result.fluxGridSha256 = sha256Float64LittleEndian(fluxGrid);
        return result;
    }

private:
    const BenchmarkConfig& m_config;
    std::vector<qulonglong> m_hits;
    qulonglong m_totalHits = 0;
    bool m_targetSideIsFront = true;
    double m_yScale = 1.;
    double m_xBinScale = 1.;
    double m_yBinScale = 1.;
};

bool writeResult(const QString& outputFileName, const QJsonObject& result, QString* errorMessage)
{
    QFileInfo info(outputFileName);
    QDir dir;
    if (!dir.mkpath(info.absolutePath()))
        return fail(errorMessage, QString("Cannot create output directory %1.").arg(info.absolutePath()));

    QSaveFile file(outputFileName);
    if (!file.open(QIODevice::WriteOnly))
        return fail(errorMessage, QString("Cannot open output file %1: %2").arg(outputFileName, file.errorString()));

    file.write(QJsonDocument(result).toJson(QJsonDocument::Indented));
    if (!file.commit())
        return fail(errorMessage, QString("Cannot write output file %1: %2").arg(outputFileName, file.errorString()));
    return true;
}
}

int BenchmarkRunner::run(const QString& configFileName, TSceneKit* scene, QString* errorMessage) const
{
    QTextStream out(stdout);

    BenchmarkConfig config;
    if (!parseConfig(configFileName, &config, errorMessage))
        return 1;
    if (config.benchmark != "benchmark_v1") {
        fail(errorMessage, QString("Unsupported benchmark: %1.").arg(config.benchmark));
        return 1;
    }
    if (!scene) {
        fail(errorMessage, "Scene is not loaded.");
        return 1;
    }

    const QDir configDir = QFileInfo(configFileName).absoluteDir();
    const QString sceneFileName = resolveRelativePath(configDir, config.sceneFile);
    const QString outputFileName = resolveRelativePath(configDir, config.outputFile);
    const QString referenceFileName = config.referenceFile.isEmpty() ? QString() : resolveRelativePath(configDir, config.referenceFile);

    ReferenceConfig reference;
    if (!parseReference(referenceFileName, &reference, errorMessage))
        return 1;

    out << "Running benchmark: " << config.benchmark << Qt::endl;
    out << "Scene: " << sceneFileName << Qt::endl;
    out << "Rays: " << config.rays << Qt::endl;
    out << "Seed: " << config.seed << Qt::endl;
    out << "Photon export: disabled" << Qt::endl;

    RayTraceOptions options;
    options.rays = config.rays;
    options.seed = config.seed;
    options.sunWidthDivisions = 100;
    options.sunHeightDivisions = 100;
    options.workerCount = qMax(1, QThread::idealThreadCount());
    options.chunkSize = 100000;

    std::vector<BenchmarkAccumulator> workerAccumulators;
    workerAccumulators.reserve(static_cast<size_t>(options.workerCount));
    for (int worker = 0; worker < options.workerCount; ++worker)
        workerAccumulators.emplace_back(config);

    RayTraceResult traceResult;
    RayTraceRunner runner;
    QString traceError;
    if (!runner.trace(scene, options, &traceResult, &traceError, [&out](const QString& message) {
            out << message << Qt::endl;
        }, RayTraceRunner::HitCallback(), [&workerAccumulators](int workerIndex) {
            return [&workerAccumulators, workerIndex](const RayTracerHit& hit) {
                workerAccumulators[static_cast<size_t>(workerIndex)].onHit(hit);
            };
        })) {
        return fail(errorMessage, QString("Benchmark trace failed: %1").arg(traceError)), 1;
    }
    if (!std::isfinite(traceResult.powerPerRay) || traceResult.powerPerRay < 0.)
        return fail(errorMessage, "Benchmark trace produced invalid power-per-ray."), 1;

    BenchmarkAccumulator accumulator(config);
    for (const BenchmarkAccumulator& workerAccumulator : workerAccumulators)
        accumulator.merge(workerAccumulator);

    const BenchmarkMetrics metrics = accumulator.metrics(traceResult.powerPerRay);
    if (!std::isfinite(metrics.totalPowerMw) ||
        !std::isfinite(metrics.minimumFluxMwM2) ||
        !std::isfinite(metrics.averageFluxMwM2) ||
        !std::isfinite(metrics.maximumFluxMwM2))
        return fail(errorMessage, "Benchmark produced non-finite metrics."), 1;

    QJsonObject result;
    result["schema_version"] = 1;
    result["benchmark"] = config.benchmark;
    result["scene_file"] = sceneFileName;
    result["rays"] = static_cast<double>(config.rays);
    result["seed"] = static_cast<double>(config.seed);
    result["elapsed_seconds"] = traceResult.elapsedSeconds;
    result["rays_per_second"] = traceResult.raysPerSecond;
    result["worker_count"] = traceResult.workerCount;
    result["chunk_count"] = static_cast<double>(traceResult.chunkCount);
    result["chunk_size"] = static_cast<double>(traceResult.chunkSize);
    result["target_side_id"] = config.targetSideId;
    result["target_bounds"] = boundsToJson(config.bounds);
    result["target_grid"] = gridToJson(config.grid);
    result["total_power_mw"] = metrics.totalPowerMw;
    result["minimum_flux_mw_m2"] = metrics.minimumFluxMwM2;
    result["average_flux_mw_m2"] = metrics.averageFluxMwM2;
    result["maximum_flux_mw_m2"] = metrics.maximumFluxMwM2;
    result["flux_grid_sha256"] = metrics.fluxGridSha256;

    if (reference.enabled) {
        const double totalPowerError = relativeErrorPercent(metrics.totalPowerMw, reference.totalPowerMw);
        const double maximumFluxError = relativeErrorPercent(metrics.maximumFluxMwM2, reference.maximumFluxMwM2);
        const bool hashMatches = metrics.fluxGridSha256.compare(reference.fluxGridSha256, Qt::CaseInsensitive) == 0;
        const bool totalPowerPass = totalPowerError <= reference.totalPowerTolerancePercent;
        const bool maximumFluxPass = maximumFluxError <= reference.maximumFluxTolerancePercent;
        result["total_power_error_percent"] = totalPowerError;
        result["maximum_flux_error_percent"] = maximumFluxError;
        result["flux_grid_hash_matches"] = hashMatches;
        result["total_power_pass"] = totalPowerPass;
        result["maximum_flux_pass"] = maximumFluxPass;
        result["flux_grid_hash_pass"] = hashMatches;
        result["benchmark_pass"] = totalPowerPass && maximumFluxPass && hashMatches;
    }

    if (!writeResult(outputFileName, result, errorMessage))
        return 1;

    out.setRealNumberNotation(QTextStream::FixedNotation);
    out.setRealNumberPrecision(6);
    out << "Benchmark completed." << Qt::endl;
    out << "elapsed_seconds: " << traceResult.elapsedSeconds << Qt::endl;
    out << "rays_per_second: " << traceResult.raysPerSecond << Qt::endl;
    out << "worker_count: " << traceResult.workerCount << Qt::endl;
    out << "chunk_count: " << traceResult.chunkCount << Qt::endl;
    out << "chunk_size: " << traceResult.chunkSize << Qt::endl;
    out << "total_power_mw: " << metrics.totalPowerMw << Qt::endl;
    out << "maximum_flux_mw_m2: " << metrics.maximumFluxMwM2 << Qt::endl;
    out << "Result written: " << outputFileName << Qt::endl;
    return 0;
}

QString BenchmarkRunner::sceneFileName(const QString& configFileName, QString* errorMessage) const
{
    BenchmarkConfig config;
    if (!parseConfig(configFileName, &config, errorMessage))
        return QString();

    return resolveRelativePath(QFileInfo(configFileName).absoluteDir(), config.sceneFile);
}
