#include <cmath>
#include <iostream>

#include <QApplication>
#include <QString>
#include <Inventor/Qt/SoQt.h>

#include "core/CorePluginRegistry.h"
#include "core/SceneInstanceBuilder.h"
#include "core/SceneLoader.h"
#include "core/TonatiuhCore.h"
#include "kernel/run/InstanceNode.h"
#include "kernel/scene/TSceneKit.h"
#include "kernel/sun/SunAperture.h"
#include "kernel/sun/SunKit.h"
#include "libraries/math/3D/Transform.h"

// Opt-in *native graphical* diagnostic. Reproduces MainWindow::UpdateLightSize()
// rather than substituting instance-tree bounds as the regular tests do.
// Any access violation here should be debugged before changing production code.
int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    SoQt::init(static_cast<QWidget*>(nullptr));
    TonatiuhCore::initializeCoin();
    CorePluginRegistry plugins;

    int differences = 0;
    const char* paths[] = {
        TONATIUHPP_EQUIVALENCE_SCENE_FILE,
        TONATIUHPP_FRESNEL_SCENE_FILE
    };

    for (const char* path : paths) {
        TonatiuhCore::setProjectSearchPaths(QString::fromUtf8(path));
        LoadedScene graphical;
        LoadedScene optical;
        QString error;
        if (!SceneLoader::readFile(QString::fromUtf8(path), &graphical, &error)
            || !SceneLoader::readFile(QString::fromUtf8(path), &optical, &error)) {
            std::cerr << "Cannot load fixture: " << error.toStdString() << '\n';
            return 2;
        }

        auto* graphicalSun = static_cast<SunKit*>(
            graphical.get()->getPart("world.sun", false));
        auto* opticalSun = static_cast<SunKit*>(
            optical.get()->getPart("world.sun", false));
        if (!graphicalSun || !opticalSun) {
            std::cerr << "Scene is missing the sun.\n";
            return 2;
        }

        // This is the exact sun-bounding-box method used by MainWindow.
        graphicalSun->setBox(graphical.get());

        SceneInstanceTree graphicalTree = SceneInstanceBuilder::build(graphical.get());
        SceneInstanceTree opticalTree = SceneInstanceBuilder::build(optical.get());
        if (!graphicalTree.layoutRoot || !opticalTree.layoutRoot)
            return 2;
        graphicalTree.layoutRoot->updateTree(Transform::Identity);
        opticalTree.layoutRoot->updateTree(Transform::Identity);
        const Box3D& box = opticalTree.layoutRoot->getBox();
        if (!box.isValid()) {
            std::cerr << "Invalid optical geometry bounds.\n";
            return 2;
        }
        opticalSun->setBox(box);

        if (!graphicalSun->findTexture(200, 200, graphicalTree.layoutRoot)
            || !opticalSun->findTexture(200, 200, opticalTree.layoutRoot)) {
            std::cerr << "Cannot sample a scene's sun aperture.\n";
            return 2;
        }

        auto* graphicalAperture = static_cast<SunAperture*>(
            graphicalSun->getPart("aperture", false));
        auto* opticalAperture = static_cast<SunAperture*>(
            opticalSun->getPart("aperture", false));
        const double graphicalArea = graphicalAperture->getArea();
        const double opticalArea = opticalAperture->getArea();

        std::cout << path << ": GUI aperture area = " << graphicalArea
                  << "; headless optical aperture area = " << opticalArea << '\n';
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

    return differences == 0 ? 0 : 1;
}
