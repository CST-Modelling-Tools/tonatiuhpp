#include <Inventor/SoDB.h>
#include <Inventor/SoInteraction.h>
#include <Inventor/Qt/SoQt.h>
#include <Inventor/Qt/viewers/SoQtExaminerViewer.h>
#include <Inventor/nodes/SoCube.h>

int main(int argc, char** argv) {
    // Initialize SoQt (also sets up a Qt application)
    QWidget* mainWin = SoQt::init(argc, argv, argv[0]);

    // Initialize Coin + Interaction
    SoDB::init();
    SoInteraction::init();

    // Create a viewer
    auto* viewer = new SoQtExaminerViewer(mainWin);

    // Create a scene containing a cube
    auto* cube = new SoCube;
    viewer->setSceneGraph(cube);
    viewer->show();

    // Main Qt loop
    SoQt::show(mainWin);
    SoQt::mainLoop();

    delete viewer;
    return 0;
}