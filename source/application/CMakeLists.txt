cmake_minimum_required(VERSION 3.28)

set(ProjectName Tonatiuh++)
project(${ProjectName} VERSION 0.1.8.16)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Header files
set(HEADERS
    calculator/CelestialWidget.h
    calculator/HorizontalWidget.h
    calculator/Image.h
    calculator/SunCalculatorDialog.h
    calculator/WorldMap.h
    commands/ActionInsert.h
    commands/CmdCopy.h
    commands/CmdCut.h
    commands/CmdDelete.h
    commands/CmdInsertNode.h
    commands/CmdPaste.h
    commands/CmdRename.h
    commands/CmdSetFieldNode.h
    commands/CmdSetFieldText.h
    main/CustomSplashScreen.h
    main/Document.h
    main/LineEditPlaceHolder.h
    main/MainWindow.h
    main/PluginManager.h
    main/UndoView.h
    parameters/ComboBoxDelegate.h
    parameters/ParametersDelegate.h
    parameters/ParametersEditor.h
    parameters/ParametersItemField.h
    parameters/ParametersItemNode.h
    parameters/ParametersModel.h
    parameters/ParametersTabs.h
    parameters/ParametersView.h
    run/FluxAnalysis.h
    run/FluxAnalysisDialog.h
    run/RayTracingDialog.h
    run/SelectSurfaceDialog.h
    script/AboutScriptDialog.h
    script/CodeEditor.h
    script/DataObject.h
    script/LineNumbers.h
    script/NodeObject.h
    script/ScriptRayTracer.h
    script/ScriptWindow.h
    script/SyntaxHighlighter.h
    script/tonatiuh_script.h
    tree/SceneDelegate.h
    tree/SceneFilter.h
    tree/SceneTreeModel.h
    tree/SceneTreeView.h
    tree/SoPathVariant.h
    updater/FileDownloader.h
    updater/UpdateDialog.h
    updater/UpdateReader.h
    view/GraphicRoot.h
    view/GraphicView.h
    view/GridNode3D.h
    view/KeyFilter.h
    view/KeyboardHook.h
    view/MenuStyle.h
    view/OverlayNode.h
    view/SeparatorStyle.h
    view/SkyNode3D.h
    view/SunNode3D.h
    view/trf.h
    widgets/AboutDialog.h
    widgets/AirDialog.h
    widgets/HelpDialog.h
    widgets/NetworkConnectionsDialog.h
    widgets/SunDialog.h
)

# Source files
set(SOURCES
    calculator/CelestialWidget.cpp
    calculator/CelestialWidget.cpp
    calculator/HorizontalWidget.cpp
    calculator/Image.cpp
    calculator/SunCalculatorDialog.cpp
    calculator/WorldMap.cpp
    commands/ActionInsert.cpp
    commands/CmdCopy.cpp
    commands/CmdCut.cpp
    commands/CmdDelete.cpp
    commands/CmdInsertNode.cpp
    commands/CmdPaste.cpp
    commands/CmdRename.cpp
    commands/CmdSetFieldNode.cpp
    commands/CmdSetFieldText.cpp
    main/CustomSplashScreen.cpp
    main/Document.cpp
    main/LineEditPlaceHolder.cpp
    main/MainWindow.cpp
    main/PluginManager.cpp
    main/UndoView.cpp
    main/main.cpp
    parameters/ComboBoxDelegate.cpp
    parameters/ParametersDelegate.cpp
    parameters/ParametersEditor.cpp
    parameters/ParametersItemField.cpp
    parameters/ParametersItemNode.cpp
    parameters/ParametersModel.cpp
    parameters/ParametersTabs.cpp
    parameters/ParametersView.cpp
    run/FluxAnalysis.cpp
    run/FluxAnalysisDialog.cpp
    run/RayTracingDialog.cpp
    run/SelectSurfaceDialog.cpp
    script/AboutScriptDialog.cpp
    script/CodeEditor.cpp
    script/DataObject.cpp
    script/LineNumbers.cpp
    script/NodeObject.cpp
    script/ScriptRayTracer.cpp
    script/ScriptWindow.cpp
    script/SyntaxHighlighter.cpp
    script/tonatiuh_script.cpp
    tree/SceneDelegate.cpp
    tree/SceneFilter.cpp
    tree/SceneTreeModel.cpp
    tree/SceneTreeView.cpp
    tree/SoPathVariant.cpp
    updater/FileDownloader.cpp
    updater/UpdateDialog.cpp
    updater/UpdateReader.cpp
    view/GraphicRoot.cpp
    view/GraphicView.cpp
    view/GridNode3D.cpp
    view/KeyFilter.cpp
    view/KeyboardHook.cpp
    view/MenuStyle.cpp
    view/OverlayNode.cpp
    view/SeparatorStyle.cpp
    view/SkyNode3D.cpp
    view/SunNode3D.cpp
    view/trf.cpp
    widgets/AboutDialog.cpp
    widgets/AirDialog.cpp
    widgets/HelpDialog.cpp
    widgets/NetworkConnectionsDialog.cpp
    widgets/SunDialog.cpp
)

# UI files
set(FORMS
    calculator/SunCalculatorDialog.ui
    run/FluxAnalysisDialog.ui
    run/RayTracingDialog.ui
    run/SelectSurfaceDialog.ui
    main/ContainerViewer.ui
    main/MainWindow.ui
    main/UndoView.ui
    parameters/ParametersEditor.ui
    script/AboutScriptDialog.ui
    script/ScriptWindow.ui
    updater/UpdateDialog.ui
    widgets/AboutDialog.ui
    widgets/AirDialog.ui
    widgets/HelpDialog.ui
    widgets/NetworkConnectionsDialog.ui
    widgets/SunDialog.ui
)

# Resources
set(RESOURCES resources.qrc)

# Create the Executable
add_executable(${ProjectName} ${SOURCES} ${HEADERS} ${FORMS} ${RESOURCES})

# Target-specific include directories
target_include_directories(${ProjectName} PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../libraries
    ${COIN3D_INCLUDE_DIR}
    ${SOQT_INCLUDE_DIR}
    $<TARGET_PROPERTY:TonatiuhLibraries,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:TonatiuhKernel,INTERFACE_INCLUDE_DIRECTORIES>
)

# Target-specific compile definitions
target_compile_definitions(${ProjectName} PRIVATE APP_VERSION="0.1.8.16")

# Link libraries
target_link_libraries(${ProjectName} 
    PRIVATE ${COIN3D_LIBRARY} 
        ${SOQT_LIBRARY} 
        Qt6::Concurrent 
        Qt6::Qml 
        Qt6::PrintSupport 
        Qt6::Gui Qt6::Network 
        TonatiuhLibraries 
        TonatiuhKernel
)

# Platform-specific configurations
if(WIN32)
    target_link_libraries(${ProjectName} PRIVATE opengl32)
    # Additional Windows-specific configurations
endif()

# Add install rules for all targets
install(TARGETS ${ProjectName}
    RUNTIME DESTINATION "${GLOBAL_INSTALL_BIN_DIR}"
    LIBRARY DESTINATION "${GLOBAL_INSTALL_BIN_DIR}"
    ARCHIVE DESTINATION "${GLOBAL_INSTALL_BIN_DIR}"
)