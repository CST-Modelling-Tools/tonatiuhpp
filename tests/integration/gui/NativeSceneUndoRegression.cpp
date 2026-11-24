// This is compiled only into the BUILD_TESTING GUI acceptance executable.
// Replays the real Fresnel GUI's nested Cut/Paste/Copy/Translate/Undo/Redo path.
// No ray tracing, reference regeneration or production-code changes.
#include "NativeSceneUndoRegression.h"

#include <cmath>

#include <QModelIndex>
#include <QTreeView>
#include <QStringList>
#include <QUndoStack>

#include <Inventor/nodes/SoNode.h>
#include "kernel/scene/TTransform.h"

#include "core/SceneInstanceBuilder.h"
#include "kernel/run/InstanceNode.h"
#include "kernel/scene/TSeparatorKit.h"
#include "main/MainWindow.h"
#include "tree/SceneTreeModel.h"

namespace {

class SceneUndoRegression final
{
public:
    explicit SceneUndoRegression(MainWindow* window)
        : m_window(window)
    {
    }

    bool run(QJsonObject* evidence, QString* error)
    {
        m_error = error;
        if (!m_window)
            return failure("MainWindow is unavailable");

        QTreeView* tree = m_window->findChild<QTreeView*>("sceneView");
        m_model = tree ? qobject_cast<SceneTreeModel*>(tree->model()) : nullptr;
        m_stack = m_window->findChild<QUndoStack*>();
        if (!m_model || !m_stack)
            return failure("Production GUI scene tree or QUndoStack is missing");

        if (!exists("//Node") || !exists("//Node/Fresnel_CSP"))
            return failure("Original Fresnel.tnhpps did not create the expected GUI scene");
        m_originalFresnel = coinNode("//Node/Fresnel_CSP");
        if (!m_originalFresnel)
            return failure("Fresnel scene node has no Coin3D identity");

        // Fresnel script itself can create undo history. The test owns only its
        // seven editing commands; this avoids Qt's ten-command limit evicting
        // steps during repeated replay.
        m_window->Select("//Node");
        m_stack->clear();
        if (m_stack->count() != 0)
            return failure("Unable to isolate the GUI edit history");
        if (!state(0))
            return false;

        m_window->Cut("//Node/Fresnel_CSP");
        if (!afterPush(1)) return false;

        m_window->Select("//Node");
        m_window->InsertNode();
        if (!afterPush(2)) return false;

        m_window->Select("//Node/Node");
        m_window->SetName("Node_01");
        if (!afterPush(3)) return false;

        m_window->Select("//Node/Node_01");
        m_window->PasteCopy();
        if (!afterPush(4)) return false;

        m_window->Select("//Node/Node_01");
        m_window->Copy();
        if (!afterPush(5)) return false;

        m_window->Select("//Node");
        m_window->PasteCopy();
        if (!afterPush(6)) return false;

        // Keep the copied node selected: the previous crash included an
        // invalid selection after Undo removed that very instance.
        m_window->Select("//Node/Node_01_1");
        m_window->SetValue("//Node/Node_01_1", "translation", "20 0 0");
        if (!afterPush(7)) return false;
        if (m_stack->count() != 7)
            return failure("GUI edit sequence did not create exactly seven undo steps");

        // Three complete reverse/forward traversals (the first begins from
        // the translated state). Check the entire scene after every action.
        for (int cycle = 0; cycle < 3; ++cycle) {
            m_window->Select("//Node/Node_01_1");
            for (int stage = 7; stage > 0; --stage) {
                if (!m_stack->canUndo())
                    return failure(QStringLiteral("Undo unexpectedly unavailable at stage ")
                                   + QString::number(stage));
                m_stack->undo();
                if (m_stack->index() != stage - 1)
                    return failure(QStringLiteral("Undo stack index mismatch at stage ")
                                   + QString::number(stage));
                if (!state(stage - 1))
                    return false;
                ++m_undoSteps;
            }
            if (m_stack->canUndo())
                return failure("Undo traversed beyond the isolated Fresnel edits");

            for (int stage = 1; stage <= 7; ++stage) {
                if (!m_stack->canRedo())
                    return failure(QStringLiteral("Redo unexpectedly unavailable at stage ")
                                   + QString::number(stage));
                m_stack->redo();
                if (m_stack->index() != stage)
                    return failure(QStringLiteral("Redo stack index mismatch at stage ")
                                   + QString::number(stage));
                if (!state(stage))
                    return false;
                ++m_redoSteps;
            }
            if (m_stack->canRedo())
                return failure("Redo unexpectedly available beyond the seven edits");
        }

        if (evidence) {
            evidence->insert("fixture", "Fresnel.tnhpps");
            evidence->insert("edit_commands", 7);
            evidence->insert("undo_steps_verified", m_undoSteps);
            evidence->insert("redo_steps_verified", m_redoSteps);
            evidence->insert("scene_states_verified", m_statesVerified);
            evidence->insert("canonical_tree_checks", m_canonicalChecks);
            evidence->insert("final_root_children", 2);
            evidence->insert("final_translation_x", 20);
            evidence->insert("nested_fresnel_clone", true);
            evidence->insert("stable_coin_node_identity", true);
        }
        return true;
    }

private:
    bool failure(const QString& message)
    {
        if (m_error)
            *m_error = message;
        return false;
    }

    SoNode* coinNode(const QString& url) const
    {
        const QModelIndex index = m_model->indexFromUrl(url);
        if (!index.isValid())
            return nullptr;
        const InstanceNode* instance = m_model->getInstance(index);
        if (!instance || instance->getURL() != url)
            return nullptr;
        return instance->getNode();
    }

    bool exists(const QString& url) const { return coinNode(url) != nullptr; }

    bool translated(double wantedX) const
    {
        const QModelIndex index = m_model->indexFromUrl("//Node/Node_01_1");
        const InstanceNode* instance = index.isValid() ? m_model->getInstance(index) : nullptr;
        const TSeparatorKit* kit = instance
            ? dynamic_cast<const TSeparatorKit*>(instance->getNode()) : nullptr;
        if (!kit)
            return false;
        // getPart(false) does not create/mutate a missing transform.
        const TTransform* transform = dynamic_cast<const TTransform*>(
            const_cast<TSeparatorKit*>(kit)->getPart("transform", false));
        if (!transform)
            return wantedX == 0.0;
        const SbVec3f& translation = transform->translation.getValue();
        return std::abs(double(translation[0]) - wantedX) < 1e-5
            && std::abs(double(translation[1])) < 1e-5
            && std::abs(double(translation[2])) < 1e-5;
    }

    QString translationReadback() const
    {
        SoNode* node = coinNode("//Node/Node_01_1");
        TSeparatorKit* kit = dynamic_cast<TSeparatorKit*>(node);
        if (!kit)
            return "missing or wrong-type TSeparatorKit";
        SoNode* part = kit->getPart("transform", false);
        const TTransform* transform = dynamic_cast<const TTransform*>(part);
        if (!transform)
            return "no TTransform part";
        const SbVec3f& value = transform->translation.getValue();
        return QStringLiteral("%1 %2 %3")
            .arg(double(value[0]), 0, 'g', 12)
            .arg(double(value[1]), 0, 'g', 12)
            .arg(double(value[2]), 0, 'g', 12);
    }

    // Reconstruct the same live Coin scene through the production headless
    // builder and compare against the actual GUI model after each edit/undo.
    bool matchesCanonical(const InstanceNode* expected, const QModelIndex& guiIndex)
    {
        const InstanceNode* actual = m_model->getInstance(guiIndex);
        if (!expected || !actual || actual->getNode() != expected->getNode()
            || actual->childCount() != expected->childCount())
            return failure(QStringLiteral("GUI instance tree differs from canonical builder at ")
                           + (expected ? expected->getURL() : QStringLiteral("<null>")));

        for (int row = 0; row < expected->childCount(); ++row) {
            const QModelIndex childIndex = m_model->index(row, 0, guiIndex);
            if (!childIndex.isValid() || m_model->parent(childIndex) != guiIndex)
                return failure(QStringLiteral("GUI instance model index mismatch at ")
                               + expected->getURL() + QStringLiteral(" row ")
                               + QString::number(row));
            if (!matchesCanonical(expected->childAt(row), childIndex))
                return false;
        }
        return true;
    }

    bool verifyCanonicalTree()
    {
        SceneInstanceTree canonical = SceneInstanceBuilder::build(m_model->getSceneKit());
        if (!canonical.sceneRoot || !matchesCanonical(canonical.sceneRoot.get(), QModelIndex()))
            return false;
        ++m_canonicalChecks;
        return true;
    }

    bool state(int stage)
    {
        const QModelIndex layout = m_model->indexFromUrl("//Node");
        if (!layout.isValid())
            return failure(QStringLiteral("Layout vanished at stage ") + QString::number(stage));

        const int rootChildren = m_model->rowCount(layout);
        const int expectedChildren = stage == 0 ? 1 : (stage < 2 ? 0 : (stage < 6 ? 1 : 2));
        const bool originalPresent = stage == 0;
        const bool insertedDefaultName = stage == 2;
        const bool renamedNode = stage >= 3;
        const bool nestedCopy = stage >= 4;
        const bool duplicate = stage >= 6;

        // Report each independent scene condition instead of grouping every
        // failure under a root-row count. Do not relax any original assertion.
        QStringList failures;
        if (rootChildren != expectedChildren)
            failures.append(QStringLiteral("root rows %1 (expected %2)")
                            .arg(rootChildren).arg(expectedChildren));
        if (exists("//Node/Fresnel_CSP") != originalPresent)
            failures.append("original Fresnel_CSP presence mismatch");
        if (exists("//Node/Node") != insertedDefaultName)
            failures.append("default Node presence mismatch");
        if (exists("//Node/Node_01") != renamedNode)
            failures.append("Node_01 presence mismatch");
        if (exists("//Node/Node_01/Fresnel_CSP") != nestedCopy)
            failures.append("nested Fresnel_CSP presence mismatch");
        if (exists("//Node/Node_01_1") != duplicate)
            failures.append("Node_01_1 presence mismatch");
        if (exists("//Node/Node_01_1/Fresnel_CSP") != duplicate)
            failures.append("copied nested Fresnel_CSP presence mismatch");
        if (duplicate && !translated(stage == 7 ? 20.0 : 0.0))
            failures.append(QStringLiteral("Node_01_1 translation mismatch: expected %1 0 0; readback %2")
                            .arg(stage == 7 ? 20 : 0)
                            .arg(translationReadback()));
        if (!failures.isEmpty()) {
            QStringList rootNames;
            for (int row = 0; row < rootChildren; ++row) {
                const QModelIndex index = m_model->index(row, 0, layout);
                const InstanceNode* instance = m_model->getInstance(index);
                const SoNode* node = instance ? instance->getNode() : nullptr;
                rootNames.append(node ? QString::fromUtf8(node->getName().getString())
                                      : QStringLiteral("<missing>"));
            }
            return failure(QStringLiteral("Scene state mismatch at edit stage %1: %2; root rows %3; children [%4]; last command [%5]")
                           .arg(stage)
                           .arg(failures.join("; "))
                           .arg(rootChildren)
                           .arg(rootNames.join(", "))
                           .arg(m_stack->index() > 0 ? m_stack->text(m_stack->index() - 1)
                                                     : QStringLiteral("none")));
        }
        if (stage == 0 && coinNode("//Node/Fresnel_CSP") != m_originalFresnel)
            return failure("Undo restored a different original Fresnel Coin3D node");
        if (stage >= 4) {
            SoNode* nested = coinNode("//Node/Node_01/Fresnel_CSP");
            if (!m_nestedFresnel)
                m_nestedFresnel = nested;
            else if (m_nestedFresnel != nested)
                return failure("Nested Fresnel clone was replaced across Redo");
        }
        if (stage >= 6) {
            SoNode* copied = coinNode("//Node/Node_01_1");
            if (!m_outerCopy)
                m_outerCopy = copied;
            else if (m_outerCopy != copied)
                return failure("Copied parent was replaced across Redo");
        }
        if (!verifyCanonicalTree())
            return false;
        ++m_statesVerified;
        return true;
    }

    bool afterPush(int stage)
    {
        if (m_stack->index() != stage || m_stack->count() != stage)
            return failure(QStringLiteral("GUI did not register editing command at stage ")
                           + QString::number(stage));
        return state(stage);
    }

    MainWindow* m_window = nullptr;
    SceneTreeModel* m_model = nullptr;
    QUndoStack* m_stack = nullptr;
    SoNode* m_originalFresnel = nullptr; // comparisons only; scene owns Coin3D refs
    SoNode* m_nestedFresnel = nullptr;
    SoNode* m_outerCopy = nullptr;
    QString* m_error = nullptr;
    int m_undoSteps = 0;
    int m_redoSteps = 0;
    int m_statesVerified = 0;
    int m_canonicalChecks = 0;
};

} // namespace

bool runNativeSceneUndoRegression(MainWindow* window, QJsonObject* evidence,
                                  QString* error)
{
    SceneUndoRegression runner(window);
    return runner.run(evidence, error);
}
