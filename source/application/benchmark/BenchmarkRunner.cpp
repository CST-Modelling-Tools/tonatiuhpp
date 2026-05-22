#include "BenchmarkRunner.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <utility>
#include <vector>

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QSaveFile>
#include <QStringList>
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
    int workerCount = 0;
    ulong chunkSize = 0;
    int targetSideId = 1;
    Bounds bounds;
    Grid grid;
    bool photonExport = false;
    QString outputFile = "benchmark_result.json";
    QString fluxGridOutputFile;
    QString fluxGridBinaryOutputFile;
    QString referenceFile;
    QString referenceFluxGridFile;
    QString referenceFluxGridBinaryFile;
};

struct ReferenceConfig
{
    bool enabled = false;
    bool hasTotalPowerMw = false;
    bool hasMaximumFluxMwM2 = false;
    bool hasFluxGridSha256 = false;
    bool hasFluxGridFile = false;
    bool hasFluxGridBinaryFile = false;
    bool hasGrid = false;
    double totalPowerMw = 0.;
    double maximumFluxMwM2 = 0.;
    QString fluxGridSha256;
    QString fluxGridFile;
    QString fluxGridBinaryFile;
    Grid grid;
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
    std::vector<double> fluxGrid;
};

struct FluxGridComparison
{
    double maximumAbsoluteErrorMwM2 = 0.;
    double maximumRelativeErrorPercent = 0.;
    double rmsErrorMwM2 = 0.;
};

bool fail(QString* errorMessage, const QString& message)
{
    if (errorMessage)
        *errorMessage = message;
    return false;
}

QString boolText(bool value)
{
    return value ? "true" : "false";
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
    if (!parsePositiveInt(object, "worker_count", &parsed.workerCount, errorMessage))
        return false;
    if (!parseULong(object, "chunk_size", true, &parsed.chunkSize, errorMessage))
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
    if (object.contains("flux_grid_output_file")) {
        if (!object.value("flux_grid_output_file").isString())
            return fail(errorMessage, "flux_grid_output_file must be a string.");
        parsed.fluxGridOutputFile = object.value("flux_grid_output_file").toString();
        if (parsed.fluxGridOutputFile.trimmed().isEmpty())
            return fail(errorMessage, "flux_grid_output_file must not be empty.");
    }
    if (object.contains("flux_grid_binary_output_file")) {
        if (!object.value("flux_grid_binary_output_file").isString())
            return fail(errorMessage, "flux_grid_binary_output_file must be a string.");
        parsed.fluxGridBinaryOutputFile = object.value("flux_grid_binary_output_file").toString();
        if (parsed.fluxGridBinaryOutputFile.trimmed().isEmpty())
            return fail(errorMessage, "flux_grid_binary_output_file must not be empty.");
    }
    if (object.contains("reference_file")) {
        if (!object.value("reference_file").isString())
            return fail(errorMessage, "reference_file must be a string.");
        parsed.referenceFile = object.value("reference_file").toString();
    }
    if (object.contains("reference_flux_grid_file")) {
        if (!object.value("reference_flux_grid_file").isString())
            return fail(errorMessage, "reference_flux_grid_file must be a string.");
        parsed.referenceFluxGridFile = object.value("reference_flux_grid_file").toString();
        if (parsed.referenceFluxGridFile.trimmed().isEmpty())
            return fail(errorMessage, "reference_flux_grid_file must not be empty.");
    }
    if (object.contains("reference_flux_grid_binary_file")) {
        if (!object.value("reference_flux_grid_binary_file").isString())
            return fail(errorMessage, "reference_flux_grid_binary_file must be a string.");
        parsed.referenceFluxGridBinaryFile = object.value("reference_flux_grid_binary_file").toString();
        if (parsed.referenceFluxGridBinaryFile.trimmed().isEmpty())
            return fail(errorMessage, "reference_flux_grid_binary_file must not be empty.");
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
    if (object.contains("total_power_mw")) {
        if (!parseFiniteDouble(object, "total_power_mw", &parsed.totalPowerMw, errorMessage))
            return false;
        parsed.hasTotalPowerMw = true;
    }
    if (object.contains("maximum_flux_mw_m2")) {
        if (!parseFiniteDouble(object, "maximum_flux_mw_m2", &parsed.maximumFluxMwM2, errorMessage))
            return false;
        parsed.hasMaximumFluxMwM2 = true;
    }
    if (object.contains("flux_grid_sha256")) {
        if (!object.value("flux_grid_sha256").isString())
            return fail(errorMessage, "reference flux_grid_sha256 must be a string.");
        parsed.fluxGridSha256 = object.value("flux_grid_sha256").toString();
        if (parsed.fluxGridSha256.trimmed().isEmpty())
            return fail(errorMessage, "reference flux_grid_sha256 must not be empty.");
        parsed.hasFluxGridSha256 = true;
    }
    if (object.contains("flux_grid_file")) {
        if (!object.value("flux_grid_file").isString())
            return fail(errorMessage, "reference flux_grid_file must be a string.");
        const QString fluxGridFile = object.value("flux_grid_file").toString();
        if (fluxGridFile.trimmed().isEmpty())
            return fail(errorMessage, "reference flux_grid_file must not be empty.");
        parsed.fluxGridFile = resolveRelativePath(QFileInfo(referenceFileName).absoluteDir(), fluxGridFile);
        parsed.hasFluxGridFile = true;
    }
    if (object.contains("flux_grid_binary_file")) {
        if (!object.value("flux_grid_binary_file").isString())
            return fail(errorMessage, "reference flux_grid_binary_file must be a string.");
        const QString fluxGridBinaryFile = object.value("flux_grid_binary_file").toString();
        if (fluxGridBinaryFile.trimmed().isEmpty())
            return fail(errorMessage, "reference flux_grid_binary_file must not be empty.");
        parsed.fluxGridBinaryFile = resolveRelativePath(QFileInfo(referenceFileName).absoluteDir(), fluxGridBinaryFile);
        parsed.hasFluxGridBinaryFile = true;
    }
    if (object.contains("target_grid")) {
        if (!object.value("target_grid").isObject())
            return fail(errorMessage, "reference target_grid must be an object.");
        const QJsonObject grid = object.value("target_grid").toObject();
        if (!parsePositiveInt(grid, "width", &parsed.grid.width, errorMessage) ||
            !parsePositiveInt(grid, "height", &parsed.grid.height, errorMessage))
            return false;
        parsed.hasGrid = true;
    }

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
    if (!parsed.hasTotalPowerMw && !parsed.hasMaximumFluxMwM2 && !parsed.hasFluxGridSha256 && !parsed.hasFluxGridFile && !parsed.hasFluxGridBinaryFile)
        return fail(errorMessage, "reference must define at least one comparable value.");

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

quint64 float64LittleEndianBits(double value)
{
    quint64 bits = 0;
    std::memcpy(&bits, &value, sizeof(value));
    return qToLittleEndian(bits);
}

QString sha256Float64LittleEndian(const std::vector<double>& values)
{
    QCryptographicHash hash(QCryptographicHash::Sha256);
    for (double value : values) {
        const quint64 bits = float64LittleEndianBits(value);
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

        result.fluxGrid.reserve(m_hits.size());

        result.minimumFluxMwM2 = std::numeric_limits<double>::max();
        for (qulonglong hits : m_hits) {
            const double flux = cellArea > 0. ? static_cast<double>(hits) * powerPerRay / cellArea / kMegawatt : 0.;
            result.fluxGrid.push_back(flux);
            result.minimumFluxMwM2 = std::min(result.minimumFluxMwM2, flux);
            result.maximumFluxMwM2 = std::max(result.maximumFluxMwM2, flux);
            result.averageFluxMwM2 += flux;
        }

        if (!result.fluxGrid.empty())
            result.averageFluxMwM2 /= static_cast<double>(result.fluxGrid.size());
        else
            result.minimumFluxMwM2 = 0.;

        result.totalPowerMw = static_cast<double>(m_totalHits) * powerPerRay / kMegawatt;
        result.fluxGridSha256 = sha256Float64LittleEndian(result.fluxGrid);
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

bool writeFluxGridCsv(const QString& outputFileName, const Grid& grid, const std::vector<double>& fluxGrid, QString* errorMessage)
{
    const size_t expectedSize = static_cast<size_t>(grid.width) * static_cast<size_t>(grid.height);
    if (fluxGrid.size() != expectedSize)
        return fail(errorMessage, "Flux grid size does not match target_grid dimensions.");

    QFileInfo info(outputFileName);
    QDir dir;
    if (!dir.mkpath(info.absolutePath()))
        return fail(errorMessage, QString("Cannot create output directory %1.").arg(info.absolutePath()));

    QSaveFile file(outputFileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return fail(errorMessage, QString("Cannot open flux grid output file %1: %2").arg(outputFileName, file.errorString()));

    QTextStream stream(&file);
    for (int row = 0; row < grid.height; ++row) {
        for (int column = 0; column < grid.width; ++column) {
            if (column > 0)
                stream << ',';
            const double value = fluxGrid[static_cast<size_t>(row) * static_cast<size_t>(grid.width) + static_cast<size_t>(column)];
            if (!std::isfinite(value))
                return fail(errorMessage, "Flux grid contains a non-finite value.");
            stream << QString::number(value, 'g', 17);
        }
        stream << '\n';
    }
    stream.flush();

    if (!file.commit())
        return fail(errorMessage, QString("Cannot write flux grid output file %1: %2").arg(outputFileName, file.errorString()));
    return true;
}

bool writeFluxGridBinary(const QString& outputFileName, const Grid& grid, const std::vector<double>& fluxGrid, QString* errorMessage)
{
    const size_t expectedSize = static_cast<size_t>(grid.width) * static_cast<size_t>(grid.height);
    if (fluxGrid.size() != expectedSize)
        return fail(errorMessage, "Flux grid size does not match target_grid dimensions.");

    QFileInfo info(outputFileName);
    QDir dir;
    if (!dir.mkpath(info.absolutePath()))
        return fail(errorMessage, QString("Cannot create output directory %1.").arg(info.absolutePath()));

    QSaveFile file(outputFileName);
    if (!file.open(QIODevice::WriteOnly))
        return fail(errorMessage, QString("Cannot open binary flux grid output file %1: %2").arg(outputFileName, file.errorString()));

    for (double value : fluxGrid) {
        if (!std::isfinite(value))
            return fail(errorMessage, "Flux grid contains a non-finite value.");
        const quint64 bits = float64LittleEndianBits(value);
        if (file.write(reinterpret_cast<const char*>(&bits), sizeof(bits)) != static_cast<qint64>(sizeof(bits)))
            return fail(errorMessage, QString("Cannot write binary flux grid output file %1: %2").arg(outputFileName, file.errorString()));
    }

    if (!file.commit())
        return fail(errorMessage, QString("Cannot write binary flux grid output file %1: %2").arg(outputFileName, file.errorString()));
    return true;
}

bool readFluxGridCsv(const QString& fileName, const Grid& grid, std::vector<double>* fluxGrid, QString* errorMessage)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return fail(errorMessage, QString("Cannot open reference flux grid file %1: %2").arg(fileName, file.errorString()));

    std::vector<double> parsed;
    parsed.reserve(static_cast<size_t>(grid.width) * static_cast<size_t>(grid.height));

    QTextStream stream(&file);
    int row = 0;
    while (!stream.atEnd()) {
        const QString line = stream.readLine();
        if (row >= grid.height)
            return fail(errorMessage, QString("Reference flux grid %1 has more than %2 rows.").arg(fileName).arg(grid.height));

        const QStringList columns = line.split(',', Qt::KeepEmptyParts);
        if (columns.size() != grid.width)
            return fail(errorMessage, QString("Reference flux grid %1 row %2 has %3 columns; expected %4.")
                .arg(fileName)
                .arg(row + 1)
                .arg(columns.size())
                .arg(grid.width));

        for (const QString& column : columns) {
            bool ok = false;
            const double value = column.trimmed().toDouble(&ok);
            if (!ok || !std::isfinite(value))
                return fail(errorMessage, QString("Reference flux grid %1 row %2 contains a non-finite or invalid value.").arg(fileName).arg(row + 1));
            parsed.push_back(value);
        }
        ++row;
    }

    if (row != grid.height)
        return fail(errorMessage, QString("Reference flux grid %1 has %2 rows; expected %3.").arg(fileName).arg(row).arg(grid.height));

    if (fluxGrid)
        *fluxGrid = std::move(parsed);
    return true;
}

bool readFluxGridBinary(const QString& fileName, const Grid& grid, std::vector<double>* fluxGrid, QString* errorMessage)
{
    const qint64 expectedValues = static_cast<qint64>(grid.width) * static_cast<qint64>(grid.height);
    const qint64 expectedBytes = expectedValues * static_cast<qint64>(sizeof(double));

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly))
        return fail(errorMessage, QString("Cannot open reference binary flux grid file %1: %2").arg(fileName, file.errorString()));
    if (file.size() != expectedBytes)
        return fail(errorMessage, QString("Reference binary flux grid file %1 is %2 bytes; expected %3 bytes.")
            .arg(fileName)
            .arg(file.size())
            .arg(expectedBytes));

    std::vector<double> parsed;
    parsed.reserve(static_cast<size_t>(expectedValues));
    for (qint64 index = 0; index < expectedValues; ++index) {
        char bytes[sizeof(double)] = {};
        if (file.read(bytes, sizeof(bytes)) != static_cast<qint64>(sizeof(bytes)))
            return fail(errorMessage, QString("Cannot read reference binary flux grid file %1: %2").arg(fileName, file.errorString()));

        const quint64 littleEndianBits = qFromLittleEndian<quint64>(reinterpret_cast<const uchar*>(bytes));
        double value = 0.;
        std::memcpy(&value, &littleEndianBits, sizeof(value));
        if (!std::isfinite(value))
            return fail(errorMessage, QString("Reference binary flux grid file %1 contains a non-finite value at index %2.").arg(fileName).arg(index));
        parsed.push_back(value);
    }

    if (fluxGrid)
        *fluxGrid = std::move(parsed);
    return true;
}

FluxGridComparison compareFluxGrids(const std::vector<double>& actual, const std::vector<double>& reference)
{
    FluxGridComparison result;
    long double squaredErrorSum = 0.;
    for (size_t index = 0; index < actual.size(); ++index) {
        const double diff = actual[index] - reference[index];
        const double absoluteError = std::abs(diff);
        result.maximumAbsoluteErrorMwM2 = std::max(result.maximumAbsoluteErrorMwM2, absoluteError);
        result.maximumRelativeErrorPercent = std::max(result.maximumRelativeErrorPercent, relativeErrorPercent(actual[index], reference[index]));
        squaredErrorSum += static_cast<long double>(diff) * static_cast<long double>(diff);
    }

    if (!actual.empty())
        result.rmsErrorMwM2 = std::sqrt(static_cast<double>(squaredErrorSum / static_cast<long double>(actual.size())));
    return result;
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
    const QString fluxGridOutputFileName = config.fluxGridOutputFile.isEmpty() ? QString() : resolveRelativePath(configDir, config.fluxGridOutputFile);
    const QString fluxGridBinaryOutputFileName = config.fluxGridBinaryOutputFile.isEmpty() ? QString() : resolveRelativePath(configDir, config.fluxGridBinaryOutputFile);
    const QString referenceFileName = config.referenceFile.isEmpty() ? QString() : resolveRelativePath(configDir, config.referenceFile);
    const QString configReferenceFluxGridFileName = config.referenceFluxGridFile.isEmpty() ? QString() : resolveRelativePath(configDir, config.referenceFluxGridFile);
    const QString configReferenceFluxGridBinaryFileName = config.referenceFluxGridBinaryFile.isEmpty() ? QString() : resolveRelativePath(configDir, config.referenceFluxGridBinaryFile);

    ReferenceConfig reference;
    if (!parseReference(referenceFileName, &reference, errorMessage))
        return 1;
    if (!configReferenceFluxGridFileName.isEmpty()) {
        if (reference.hasFluxGridFile && QFileInfo(reference.fluxGridFile).absoluteFilePath() != QFileInfo(configReferenceFluxGridFileName).absoluteFilePath())
            return fail(errorMessage, "reference_flux_grid_file conflicts with reference flux_grid_file."), 1;
        reference.enabled = true;
        reference.hasFluxGridFile = true;
        reference.fluxGridFile = configReferenceFluxGridFileName;
    }
    if (!configReferenceFluxGridBinaryFileName.isEmpty()) {
        if (reference.hasFluxGridBinaryFile && QFileInfo(reference.fluxGridBinaryFile).absoluteFilePath() != QFileInfo(configReferenceFluxGridBinaryFileName).absoluteFilePath())
            return fail(errorMessage, "reference_flux_grid_binary_file conflicts with reference flux_grid_binary_file."), 1;
        reference.enabled = true;
        reference.hasFluxGridBinaryFile = true;
        reference.fluxGridBinaryFile = configReferenceFluxGridBinaryFileName;
    }

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
    options.workerCount = config.workerCount > 0 ? config.workerCount : qMax(1, QThread::idealThreadCount());
    options.chunkSize = config.chunkSize > 0 ? config.chunkSize : 10000;

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

    if (!fluxGridOutputFileName.isEmpty() && !writeFluxGridCsv(fluxGridOutputFileName, config.grid, metrics.fluxGrid, errorMessage))
        return 1;
    if (!fluxGridBinaryOutputFileName.isEmpty() && !writeFluxGridBinary(fluxGridBinaryOutputFileName, config.grid, metrics.fluxGrid, errorMessage))
        return 1;

    std::vector<double> referenceFluxGrid;
    QString referenceFluxGridSha256;
    FluxGridComparison fluxGridComparison;
    const bool hasReferenceGrid = reference.hasFluxGridBinaryFile || reference.hasFluxGridFile;
    const bool usingBinaryReferenceGrid = reference.hasFluxGridBinaryFile;
    const Grid referenceGrid = reference.hasGrid ? reference.grid : config.grid;
    if (hasReferenceGrid) {
        if (usingBinaryReferenceGrid) {
            if (!readFluxGridBinary(reference.fluxGridBinaryFile, referenceGrid, &referenceFluxGrid, errorMessage))
                return 1;
        } else if (!readFluxGridCsv(reference.fluxGridFile, referenceGrid, &referenceFluxGrid, errorMessage)) {
            return 1;
        }
        if (referenceFluxGrid.size() != metrics.fluxGrid.size())
            return fail(errorMessage, "Reference flux grid size does not match computed flux grid size."), 1;

        referenceFluxGridSha256 = sha256Float64LittleEndian(referenceFluxGrid);
        if (reference.hasFluxGridSha256 && referenceFluxGridSha256.compare(reference.fluxGridSha256, Qt::CaseInsensitive) != 0)
            return fail(errorMessage, "Reference flux_grid_sha256 does not match reference flux_grid_file."), 1;
        fluxGridComparison = compareFluxGrids(metrics.fluxGrid, referenceFluxGrid);
    }

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
    if (!fluxGridOutputFileName.isEmpty())
        result["flux_grid_output_file"] = fluxGridOutputFileName;
    if (!fluxGridBinaryOutputFileName.isEmpty())
        result["flux_grid_binary_output_file"] = fluxGridBinaryOutputFileName;

    if (reference.enabled) {
        bool benchmarkPass = true;
        if (reference.hasTotalPowerMw) {
            const double totalPowerError = relativeErrorPercent(metrics.totalPowerMw, reference.totalPowerMw);
            const bool totalPowerPass = totalPowerError <= reference.totalPowerTolerancePercent;
            result["total_power_error_percent"] = totalPowerError;
            result["total_power_pass"] = totalPowerPass;
            benchmarkPass = benchmarkPass && totalPowerPass;
        }
        if (reference.hasMaximumFluxMwM2) {
            const double maximumFluxError = relativeErrorPercent(metrics.maximumFluxMwM2, reference.maximumFluxMwM2);
            const bool maximumFluxPass = maximumFluxError <= reference.maximumFluxTolerancePercent;
            result["maximum_flux_error_percent"] = maximumFluxError;
            result["maximum_flux_pass"] = maximumFluxPass;
            benchmarkPass = benchmarkPass && maximumFluxPass;
        }
        if (reference.hasFluxGridSha256 || hasReferenceGrid) {
            const QString referenceHash = reference.hasFluxGridSha256 ? reference.fluxGridSha256 : referenceFluxGridSha256;
            const bool hashMatches = metrics.fluxGridSha256.compare(referenceHash, Qt::CaseInsensitive) == 0;
            result["flux_grid_hash_matches"] = hashMatches;
            result["flux_grid_hash_pass"] = hashMatches;
            benchmarkPass = benchmarkPass && hashMatches;
        }
        if (hasReferenceGrid) {
            if (usingBinaryReferenceGrid)
                result["reference_flux_grid_binary_file"] = reference.fluxGridBinaryFile;
            else
                result["reference_flux_grid_file"] = reference.fluxGridFile;
            result["maximum_flux_grid_absolute_error_mw_m2"] = fluxGridComparison.maximumAbsoluteErrorMwM2;
            result["maximum_flux_grid_relative_error_percent"] = fluxGridComparison.maximumRelativeErrorPercent;
            result["rms_flux_grid_error_mw_m2"] = fluxGridComparison.rmsErrorMwM2;
        }
        result["benchmark_pass"] = benchmarkPass;
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
    if (reference.enabled) {
        out << "Comparison:" << Qt::endl;
        out << "benchmark_pass: " << boolText(result.value("benchmark_pass").toBool()) << Qt::endl;
        if (result.contains("total_power_error_percent") && result.contains("total_power_pass")) {
            out << "total_power_error_percent: " << result.value("total_power_error_percent").toDouble()
                << ", total_power_pass: " << boolText(result.value("total_power_pass").toBool()) << Qt::endl;
        }
        if (result.contains("maximum_flux_error_percent") && result.contains("maximum_flux_pass")) {
            out << "maximum_flux_error_percent: " << result.value("maximum_flux_error_percent").toDouble()
                << ", maximum_flux_pass: " << boolText(result.value("maximum_flux_pass").toBool()) << Qt::endl;
        }
        if (result.contains("flux_grid_hash_matches") && result.contains("flux_grid_hash_pass")) {
            out << "flux_grid_hash_matches: " << boolText(result.value("flux_grid_hash_matches").toBool())
                << ", flux_grid_hash_pass: " << boolText(result.value("flux_grid_hash_pass").toBool()) << Qt::endl;
        }
    }
    if (!fluxGridOutputFileName.isEmpty())
        out << "Flux grid written: " << fluxGridOutputFileName << Qt::endl;
    if (!fluxGridBinaryOutputFileName.isEmpty())
        out << "Binary flux grid written: " << fluxGridBinaryOutputFileName << Qt::endl;
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
