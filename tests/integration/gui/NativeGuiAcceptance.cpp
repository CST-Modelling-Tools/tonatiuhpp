// M5: real Qt-widget presenter acceptance. Compiled into the GUI only while
// BUILD_TESTING is true, and inert without an explicit test-mode environment.
// No changes to the scientific kernel, plugins, ray propagation or RNG.
#include "NativeGuiAcceptance.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <functional>
#include <utility>

#include <QAbstractButton>
#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QDebug>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPointer>
#include <QPushButton>
#include <QSaveFile>
#include <QSpinBox>
#include <QTimer>
#include <QWidget>

#include "main/MainWindow.h"
#include "libraries/QCustomPlot/qcustomplot.h"

namespace {

class InteractiveAcceptance final
{
public:
    explicit InteractiveAcceptance(MainWindow* window, QString mode, QString report,
                                   QString outputDir, QString scriptFile)
        : m_main(window), m_mode(std::move(mode)), m_report(std::move(report)),
          m_outputDir(std::move(outputDir)), m_scriptFile(std::move(scriptFile))
    {
        m_data.insert("schema", "tonatiuhpp.m5.native-gui-presenter.v1");
        m_data.insert("mode", m_mode);
        m_data.insert("status", "RUNNING");
        m_data.insert("gui_backend", "QApplication/MainWindow/actual Qt dialogs");
        writeReport();
    }

    void start()
    {
        // Deliberately bounded; fail rather than leaving a modal test running
        // indefinitely. The caller can inspect the preserved report.
        QTimer::singleShot(120000, [this] {
            if (!m_finished) fail("Timed out while exercising real GUI widgets");
        });
        if (!m_main || !m_main->isVisible()) {
            fail("MainWindow not visible after GUI initialization");
            return;
        }
        if (m_mode == "interactive-file") {
            runRayDialog(true, 0, [this] { verifyInteractiveFile(); });
        } else if (m_mode == "seeded-interception") {
            runRayDialog(false, 0, [this] { runEditorScript(); });
        } else if (m_mode == "interactive-flux") {
            // A one-ray no-export GUI Run configures the same seed used later
            // by the *actual* interactive Flux Analysis QDialog.
            runRayDialog(false, 123456789, [this] {
                runEditorScript([this] { runFluxDialog(); });
            });
        } else {
            fail("Unknown test-only GUI acceptance mode");
        }
    }

private:
    template<class T>
    T* required(QWidget* parent, const char* objectName)
    {
        T* child = parent ? parent->findChild<T*>(objectName) : nullptr;
        if (!child) fail(QString("Missing real Qt widget: %1").arg(objectName));
        return child;
    }

    void writeReport()
    {
        QSaveFile output(m_report);
        if (!output.open(QIODevice::WriteOnly)) return;
        const QByteArray json = QJsonDocument(m_data).toJson(QJsonDocument::Indented);
        if (output.write(json) != json.size() || !output.commit()) {
            qWarning("M5 GUI acceptance could not write its report");
        }
    }

    void finish(const QString& state, const QString& message, int exitCode)
    {
        if (m_finished) return;
        m_finished = true;
        m_data.insert("status", state);
        m_data.insert("message", message);
        writeReport();
        fprintf(exitCode == 0 ? stdout : stderr, "M5 native GUI %s: %s\n",
                state.toUtf8().constData(), message.toUtf8().constData());
        fflush(exitCode == 0 ? stdout : stderr);
        QTimer::singleShot(0, [exitCode] { QCoreApplication::exit(exitCode); });
    }

    void fail(const QString& why) { finish("FAIL", why, 3); }
    void pass(const QString& what) { finish("PASS", what, 0); }

    QDialog* modal(const QString& expected, int tries)
    {
        if (m_finished) return nullptr;
        auto* current = QApplication::activeModalWidget();
        if (auto* warning = qobject_cast<QMessageBox*>(current)) {
            fail(QString("GUI displayed unexpected QMessageBox: %1").arg(warning->text()));
            warning->reject();
            return nullptr;
        }
        if (current && current->objectName() == expected)
            return qobject_cast<QDialog*>(current);
        if (tries >= 75)
            fail(QString("Actual GUI modal '%1' not visible").arg(expected));
        return nullptr;
    }

    void runRayDialog(bool exportFile, unsigned seed, std::function<void()> done)
    {
        if (m_finished || !m_main) return;
        m_afterRay = std::move(done);
        m_rayExport = exportFile;
        m_seed = seed;
        m_rayDialogConfigured = false;
        QAction* action = m_main->findChild<QAction*>("actionRun");
        if (!action) { fail("Real GUI Ray Tracing QAction unavailable"); return; }
        QTimer::singleShot(100, [this] { configureRayDialog(0); });
        action->trigger(); // Enters actual RayTracingDialog::exec and GUI Run().
        if (m_finished) return;
        if (!m_rayDialogConfigured) {
            fail("Ray Tracing dialog did not accept the test inputs");
            return;
        }
        if (m_afterRay) m_afterRay();
    }

    void configureRayDialog(int tries)
    {
        if (m_finished) return;
        QDialog* dialog = modal("RayTracingDialog", tries);
        if (!dialog) {
            if (!m_finished) QTimer::singleShot(100, [this, tries] { configureRayDialog(tries + 1); });
            return;
        }
        auto* rays = required<QSpinBox>(dialog, "raysNumberSpin");
        auto* gridW = required<QSpinBox>(dialog, "raysPlaneWidthSpin");
        auto* gridH = required<QSpinBox>(dialog, "raysPlaneHeightSpin");
        auto* checkSeed = required<QCheckBox>(dialog, "fixedSeedCheck");
        auto* editSeed = required<QLineEdit>(dialog, "fixedSeedEdit");
        auto* exportType = required<QComboBox>(dialog, "storeTypeCombo");
        auto* buttons = required<QDialogButtonBox>(dialog, "buttonBox");
        if (m_finished) return;
        rays->setValue(m_rayExport ? 2048 : 1);
        gridW->setValue(m_rayExport ? 200 : (m_mode == "interactive-flux" ? 64 : 200));
        gridH->setValue(m_rayExport ? 200 : (m_mode == "interactive-flux" ? 48 : 200));
        checkSeed->setChecked(true);
        editSeed->setText(QString::number(m_seed));
        const int selected = exportType->findText(m_rayExport ? "File" : "No export");
        if (selected < 0) {
            fail("Real GUI plugin factory missing File or No export entry");
            return;
        }
        exportType->setCurrentIndex(selected);

        if (m_rayExport) {
            auto* directory = required<QLineEdit>(dialog, "directoryEdit");
            auto* file = required<QLineEdit>(dialog, "fileEdit");
            auto* segmented = required<QCheckBox>(dialog, "photonsCheck");
            auto* coords = required<QCheckBox>(dialog, "saveCoordinatesCheck");
            auto* global = required<QAbstractButton>(dialog, "saveGlobalRadio");
            auto* surface = required<QCheckBox>(dialog, "saveSurfaceCheck");
            auto* side = required<QCheckBox>(dialog, "saveSideCheck");
            auto* photonIds = required<QCheckBox>(dialog, "savePhotonsCheck");
            auto* all = required<QAbstractButton>(dialog, "surfaceAllRadio");
            auto* append = required<QAbstractButton>(dialog, "photonBufferAppendRadio");
            if (m_finished) return;
            directory->setText(m_outputDir);
            file->setText("PhotonMap");
            segmented->setChecked(false); // Established unsegmented output.
            coords->setChecked(true);
            global->setChecked(true);
            surface->setChecked(true);
            side->setChecked(true);
            photonIds->setChecked(false);
            all->setChecked(true);
            append->setChecked(false);
            if (!QFileInfo(m_outputDir).isDir()) {
                fail("The disposable File-export output directory does not exist");
                return;
            }
            m_data.insert("export_dir", m_outputDir);
        }
        if (m_finished) return;
        auto* ok = buttons->button(QDialogButtonBox::Ok);
        if (!ok || !ok->isEnabled()) { fail("Real GUI Ray Tracing OK disabled"); return; }
        m_rayDialogConfigured = true;
        m_data.insert("ray_dialog_accepted", true);
        m_data.insert("fixed_seed", static_cast<qint64>(m_seed));
        writeReport();
        ok->click(); // Full GUI Run follows after this dialog exits.
    }

    void verifyInteractiveFile()
    {
        if (m_finished) return;
        const QFileInfo data(m_outputDir + "/PhotonMap.dat");
        const QFileInfo parameters(m_outputDir + "/PhotonMap_parameters.txt");
        if (!data.isFile() || !parameters.isFile() || data.size() < 48 || parameters.size() <= 0) {
            fail("Interactive GUI File exporter did not produce valid nonempty output files");
            return;
        }
        m_data.insert("photon_bytes", static_cast<qint64>(data.size()));
        m_data.insert("parameters_bytes", static_cast<qint64>(parameters.size()));
        m_data.insert("photon_records", static_cast<qint64>(data.size() / 48));
        pass("Actual RayTracingDialog configured File plugin and completed bounded GUI export");
    }

    void runEditorScript(std::function<void()> after = {})
    {
        if (m_finished || !m_main) return;
        if (m_scriptFile.isEmpty() || !QFileInfo::exists(m_scriptFile)) {
            fail("Original bounded script fixture missing from disposable folder");
            return;
        }
        m_main->openFileScript(m_scriptFile); // Actual native GUI Script Editor.
        QWidget* editor = nullptr;
        for (QWidget* candidate : QApplication::topLevelWidgets()) {
            if (candidate && candidate->isVisible() &&
                candidate->inherits("ScriptWindow") &&
                candidate->windowTitle().contains(QFileInfo(m_scriptFile).fileName())) {
                editor = candidate;
                break;
            }
        }
        if (!editor) { fail("Actual GUI Script Editor window not visible"); return; }
        QAction* run = editor->findChild<QAction*>("actionRunScript");
        if (!run) { fail("Actual GUI Script Editor Run action missing"); return; }
        run->trigger(); // ScriptWindow::runScript(), not headless evaluation.
        if (m_finished) return;
        const QString scriptDir = QFileInfo(m_scriptFile).absolutePath();
        if (m_mode == "seeded-interception") {
            if (!QFileInfo::exists(scriptDir + "/m3_3_parity_result.csv")) {
                fail("Seed-zero GUI editor did not generate M3.3 interception CSV");
                return;
            }
            editor->close();
            pass("Actual GUI Script Editor ran original M3.3 interception with fixed master seed 0");
        } else {
            if (!QFileInfo::exists(scriptDir + "/m3_flux_front.txt") ||
                !QFileInfo::exists(scriptDir + "/m3_flux_back.txt")) {
                fail("GUI Editor failed fixed-seed front/back Flux script setup");
                return;
            }
            editor->close();
            if (after) QTimer::singleShot(0, [after] { after(); });
        }
    }

    void runFluxDialog()
    {
        if (m_finished || !m_main) return;
        QAction* action = m_main->findChild<QAction*>("actionRunFlux");
        if (!action) { fail("Real GUI Flux Analysis QAction unavailable"); return; }
        QTimer::singleShot(100, [this] { configureFluxDialog(0); });
        action->trigger(); // Actual FluxAnalysisDialog::exec() and run().
        if (!m_finished) fail("Flux Analysis dialog returned without scientific acceptance");
    }

    void configureFluxDialog(int tries)
    {
        if (m_finished) return;
        QDialog* dialog = modal("FluxAnalysisDialog", tries);
        if (!dialog) {
            if (!m_finished) QTimer::singleShot(100, [this, tries] { configureFluxDialog(tries + 1); });
            return;
        }
        auto* target = required<QLineEdit>(dialog, "surfaceEdit");
        auto* side = required<QComboBox>(dialog, "surfaceSideCombo");
        auto* rays = required<QSpinBox>(dialog, "raysSpin");
        auto* cellsX = required<QSpinBox>(dialog, "surfaceXSpin");
        auto* cellsY = required<QSpinBox>(dialog, "surfaceYSpin");
        auto* run = required<QPushButton>(dialog, "raysButton");
        if (m_finished) return;
        target->setText("//Node/M3_Flux_Parity/Receiver/Shape");
        if (!QMetaObject::invokeMethod(dialog, "SurfaceChanged", Qt::DirectConnection)) {
            fail("Actual Qt Flux Analysis surface selection slot failed");
            return;
        }
        const int front = side->findText("front");
        if (front < 0) { fail("Front side unavailable from real Flux GUI"); return; }
        side->setCurrentIndex(front);
        rays->setValue(4096);
        cellsX->setValue(8);
        cellsY->setValue(8);
        if (!run->isEnabled()) { fail("Flux Analysis GUI Run button disabled"); return; }
        run->click(); // Actual interactive FluxAnalysis::run + plot rendering.
        if (m_finished) return;
        auto* power = required<QLabel>(dialog, "powerTotal");
        auto* maximum = required<QLabel>(dialog, "fluxMax");
        auto* plot = required<QCustomPlot>(dialog, "plotFxy");
        if (m_finished) return;
        bool okPower = false, okMax = false;
        const double p = power->text().toDouble(&okPower);
        const double m = maximum->text().toDouble(&okMax);
        if (!okPower || !okMax || !std::isfinite(p) || !std::isfinite(m) || p <= 0 || m <= 0) {
            fail(QString("Interactive Flux Analysis did not produce finite positive power/peak: %1 / %2")
                 .arg(power->text(), maximum->text()));
            dialog->reject();
            return;
        }
        QCPColorMap* colorMap = nullptr;
        for (int i = 0; i < plot->plottableCount(); ++i) {
            colorMap = dynamic_cast<QCPColorMap*>(plot->plottable(i));
            if (colorMap) break;
        }
        if (!colorMap || !colorMap->data() ||
            colorMap->data()->keySize() != 8 || colorMap->data()->valueSize() != 8) {
            fail("The actual Qt Flux Analysis plot has no populated 8x8 color map");
            dialog->reject();
            return;
        }
        QJsonArray rows;
        double cellsSum = 0, cellsMax = 0;
        for (int y = 0; y < 8; ++y) {
            QJsonArray row;
            for (int x = 0; x < 8; ++x) {
                double value = colorMap->data()->cell(x, y);
                if (!std::isfinite(value) || value < 0) {
                    fail("Real Qt flux plot contains nonfinite/negative cells");
                    dialog->reject();
                    return;
                }
                cellsSum += value;
                cellsMax = std::max(cellsMax, value);
                row.append(value);
            }
            rows.append(row);
        }
        if (cellsSum <= 0 || cellsMax <= 0) {
            fail("Real Qt Flux plot contains no positive scientific signal");
            dialog->reject();
            return;
        }
        m_data.insert("plot_cells_yx", rows);
        m_data.insert("plot_sum", cellsSum);
        m_data.insert("plot_max", cellsMax);
        m_data.insert("displayed_power_w", p);
        m_data.insert("displayed_peak_flux", m);
        const QString png = QFileInfo(m_report).dir().filePath("interactive_flux_plot.png");
        if (!plot->savePng(png, 960, 600)) {
            fail("Actual interactive flux plot could not be saved as PNG evidence");
            dialog->reject();
            return;
        }
        m_data.insert("plot_png", png);
        dialog->accept();
        pass("Actual FluxAnalysisDialog Run computed 8x8 positive Qt color map and statistics");
    }

    QPointer<MainWindow> m_main;
    QString m_mode, m_report, m_outputDir, m_scriptFile;
    QJsonObject m_data;
    std::function<void()> m_afterRay;
    bool m_finished = false;
    bool m_rayDialogConfigured = false;
    bool m_rayExport = false;
    unsigned m_seed = 0;
};

} // namespace

void installNativeGuiAcceptance(MainWindow* mainWindow)
{
    const QString report = qEnvironmentVariable("TONATIUHPP_M5_GUI_NATIVE_REPORT");
    if (report.isEmpty()) return; // No effect during normal operation.
    const QString mode = qEnvironmentVariable("TONATIUHPP_M5_GUI_NATIVE_MODE");
    const QString outputDir = qEnvironmentVariable("TONATIUHPP_M5_GUI_NATIVE_EXPORT_DIR");
    const QString script = qEnvironmentVariable("TONATIUHPP_M5_GUI_NATIVE_SCRIPT");
    auto* test = new InteractiveAcceptance(mainWindow, mode, report, outputDir, script);
    QTimer::singleShot(200, [test] { test->start(); });
}
