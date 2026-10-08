#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>

#include <QApplication>
#include <QString>
#include <Inventor/Qt/SoQt.h>
#include <Inventor/SoDB.h>
#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/sensors/SoSensorManager.h>

#include "core/CorePluginRegistry.h"
#include "core/SceneInstanceBuilder.h"
#include "core/SceneLoader.h"
#include "core/TonatiuhCore.h"
#include "kernel/run/InstanceNode.h"
#include "kernel/scene/TSceneKit.h"
#include "kernel/scene/TSeparatorKit.h"
#include "kernel/sun/SunAperture.h"
#include "kernel/sun/SunKit.h"
#include "libraries/math/3D/Transform.h"

namespace
{
void checkpoint(const char* stage, const char* path = nullptr)
{
    std::cerr << "[gui-sun] " << stage;
    if (path)
        std::cerr << " | fixture: " << path;
    std::cerr << std::endl;
}

void printCoinBounds(const char* stage, TSceneKit* scene)
{
    SoGetBoundingBoxAction action{SbViewportRegion()};
    action.apply(scene->getLayout());
    const SbBox3f box = action.getBoundingBox();
    std::cerr << "[gui-sun] " << stage << ": ";
    if (box.isEmpty()) {
        std::cerr << "EMPTY" << std::endl;
        return;
    }
    const SbVec3f& a = box.getMin();
    const SbVec3f& b = box.getMax();
    std::cerr << std::setprecision(12)
              << "min=(" << a[0] << ", " << a[1] << ", " << a[2] << ")"
              << ", max=(" << b[0] << ", " << b[1] << ", " << b[2] << ")"
              << std::endl;
}

void printOpticalBounds(const char* stage, const Box3D& box)
{
    const vec3d& a = box.min();
    const vec3d& b = box.max();
    std::cerr << "[gui-sun] " << stage << ": "
              << std::setprecision(12)
              << "min=(" << a.x << ", " << a.y << ", " << a.z << ")"
              << ", max=(" << b.x << ", " << b.y << ", " << b.z << ")"
              << std::endl;
}
}

// Opt-in *native graphical* diagnostic. Reproduces MainWindow::UpdateLightSize()
// rather than substituting instance-tree bounds as the regular tests do.
// Any access violation here should be debugged before changing production code.
int main(int argc, char** argv)
{
    checkpoint("enter main");
    QApplication app(argc, argv);
    checkpoint("QApplication initialized");
    checkpoint("before SoQt::init");
    SoQt::init(static_cast<QWidget*>(nullptr));
    checkpoint("SoQt initialized");
    checkpoint("before TonatiuhCore::initializeCoin");
    TonatiuhCore::initializeCoin();
    checkpoint("Coin initialized");
    checkpoint("before CorePluginRegistry");
    CorePluginRegistry plugins;
    checkpoint("built-in scene types registered");

    int differences = 0;
    const char* paths[] = {
        TONATIUHPP_EQUIVALENCE_SCENE_FILE,
        TONATIUHPP_FRESNEL_SCENE_FILE
    };

    for (const char* path : paths) {
        checkpoint("begin fixture", path);
        TonatiuhCore::setProjectSearchPaths(QString::fromUtf8(path));
        LoadedScene graphical;
        LoadedScene optical;
        QString error;
        if (!SceneLoader::readFile(QString::fromUtf8(path), &graphical, &error)
            || !SceneLoader::readFile(QString::fromUtf8(path), &optical, &error)) {
            std::cerr << "Cannot load fixture: " << error.toStdString() << '\n';
            return 2;
        }
        checkpoint("both scene files loaded", path);

        auto* graphicalSun = static_cast<SunKit*>(
            graphical.get()->getPart("world.sun", false));
        auto* opticalSun = static_cast<SunKit*>(
            optical.get()->getPart("world.sun", false));
        if (!graphicalSun || !opticalSun) {
            std::cerr << "Scene is missing the sun.\n";
            return 2;
        }
        checkpoint("sun nodes located", path);

        // The production GUI processes Coin field sensors via its event loop.
        // This standalone executable measures whether pending display-mesh
        // updates account for the aperture-area difference.
        printCoinBounds("Coin preview bounds BEFORE sensor queue", graphical.get());
        checkpoint("processing pending Coin sensor queue", path);
        SoDB::getSensorManager()->processDelayQueue(true);
        checkpoint("Coin sensor queue processed", path);
        printCoinBounds("Coin preview bounds AFTER sensor queue", graphical.get());

        // This is the exact sun-bounding-box method used by MainWindow.
        checkpoint("before SunKit::setBox(TSceneKit*)", path);
        graphicalSun->setBox(graphical.get());
        checkpoint("after SunKit::setBox(TSceneKit*)", path);

        checkpoint("before SceneInstanceBuilder::build", path);
        SceneInstanceTree graphicalTree = SceneInstanceBuilder::build(graphical.get());
        SceneInstanceTree opticalTree = SceneInstanceBuilder::build(optical.get());
        checkpoint("scene instance trees built", path);
        if (!graphicalTree.layoutRoot || !opticalTree.layoutRoot)
            return 2;
        graphicalTree.layoutRoot->updateTree(Transform::Identity);
        opticalTree.layoutRoot->updateTree(Transform::Identity);
        checkpoint("instance bounds updated", path);
        const Box3D& box = opticalTree.layoutRoot->getBox();
        if (!box.isValid()) {
            std::cerr << "Invalid optical geometry bounds.\n";
            return 2;
        }
        printOpticalBounds("Ray-tracing instance bounds", box);
        opticalSun->setBox(box);
        checkpoint("optical sun bounds set", path);

        checkpoint("before both SunKit::findTexture calls", path);
        if (!graphicalSun->findTexture(200, 200, graphicalTree.layoutRoot)
            || !opticalSun->findTexture(200, 200, opticalTree.layoutRoot)) {
            std::cerr << "Cannot sample a scene's sun aperture.\n";
            return 2;
        }
        checkpoint("both sun aperture textures found", path);

        auto* graphicalAperture = static_cast<SunAperture*>(
            graphicalSun->getPart("aperture", false));
        auto* opticalAperture = static_cast<SunAperture*>(
            opticalSun->getPart("aperture", false));
        checkpoint("before reading sun aperture areas", path);
        const double graphicalArea = graphicalAperture->getArea();
        const double opticalArea = opticalAperture->getArea();

        std::cout << path << ": GUI aperture area = " << graphicalArea
                  << "; headless optical aperture area = " << opticalArea << std::endl;
        checkpoint("areas printed", path);
        if (!std::isfinite(graphicalArea) || !std::isfinite(opticalArea)
            || graphicalArea <= 0. || opticalArea <= 0.) {
            std::cerr << "Invalid aperture areas.\n";
            return 2;
        }

        const double tolerance = 1e-9 * std::max({1., graphicalArea, opticalArea});
        if (std::abs(graphicalArea - opticalArea) > tolerance) {
            std::cerr << "GUI and headless aperture calculations differ.\n";
            ++differences;
        }
    }

    checkpoint(differences == 0 ? "completed: matching areas" : "completed: area mismatch");
    return differences == 0 ? 0 : 1;
}
