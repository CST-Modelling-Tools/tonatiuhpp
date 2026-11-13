cmake_minimum_required(VERSION 3.28)

# Logical target name may include '+'; we'll give the on-disk exe a simpler name.
set(ProjectName tonatiuhpp-gui)
set(ProjectVersion "${CMAKE_PROJECT_VERSION}")
project(${ProjectName} VERSION ${ProjectVersion})

# C++ standard (global is already set, but harmless here)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# ----------------------------
# Headers
# ----------------------------
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
    core/CorePluginRegistry.h
    core/SimulationConfig.h
    core/GuiTraceCompletion.h
    core/GuiTraceRunInputs.h
    core/FluxGridComputation.h
    core/GuiTraceSessionExecution.h
    core/TonatiuhCore.h
    headless/HeadlessScriptFluxBackend.h
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
    run/FluxAnalysisTsvWriter.h
    run/FluxAnalysisDialog.h
    run/GuiTraceSeed.h
    run/RayTracingDialog.h
    run/SelectSurfaceDialog.h
    script/AboutScriptDialog.h
    script/CodeEditor.h
    script/GuiScriptSceneAdapter.h
    script/ScriptRunExecutionService.h
    script/ScriptFluxAnalysisRequest.h
    script/LineNumbers.h
    script/ScriptRayTracer.h
    script/ScriptWindow.h
    script/SyntaxHighlighter.h
    script/tonatiuh_script.h
    tree/SceneDelegate.h
    tree/SceneFilter.h
    tree/SceneTreeModel.h
    tree/SceneTreeView.h
    tree/SoPathVariant.h
    updater/IfwUpdateService.h
    updater/UpdateDialog.h
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

# ----------------------------
# Sources
# ----------------------------
set(SOURCES
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
    core/CorePluginRegistry.cpp
    core/TonatiuhCore.cpp
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
    script/GuiScriptSceneAdapter.cpp
    script/LineNumbers.cpp
    script/ScriptRayTracer.cpp
    script/ScriptWindow.cpp
    script/SyntaxHighlighter.cpp
    script/tonatiuh_script.cpp
    tree/SceneDelegate.cpp
    tree/SceneFilter.cpp
    tree/SceneTreeModel.cpp
    tree/SceneTreeView.cpp
    tree/SoPathVariant.cpp
    updater/IfwUpdateService.cpp
    updater/UpdateDialog.cpp
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

# ----------------------------
# UI + Resources
# ----------------------------
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

set(RESOURCES resources.qrc)

# M5.1A: physically compile shared services outside the GUI executable.
# This is a *direct source/target* boundary, not yet a Widgets/SoQt-free
# transitive link closure. The existing kernel, libraries and plugin ABI
# still require GUI dependencies; M5.1B must remove those before M5 exit.
set(TONATIUHPP_M5_SIMULATION_SOURCES
    core/NativeTraceSignature.cpp
    core/NativeRayHistorySignature.cpp
    core/RayTraceExecutor.cpp
    core/SceneInstanceBuilder.cpp
    core/SceneLoader.cpp
    core/SimulationSession.cpp
    core/TracePreparation.cpp
)
set(TONATIUHPP_M5_SIMULATION_HEADERS
    core/NativeTraceSignature.h
    core/NativeRayHistorySignature.h
    core/RayTraceExecutor.h
    core/SceneInstanceBuilder.h
    core/SceneLoader.h
    core/SimulationSession.h
    core/TracePreparation.h
)
add_library(TonatiuhSimulation STATIC
    ${TONATIUHPP_M5_SIMULATION_SOURCES}
    ${TONATIUHPP_M5_SIMULATION_HEADERS}
)
target_include_directories(TonatiuhSimulation PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../libraries
    ${COIN3D_INCLUDE_DIR}
)
# Qt6::Gui is still needed by legacy kernel factory headers (QIcon).
# Removing QtGui and the transitive Widgets/SoQt link debt is M5.1B work.
target_link_libraries(TonatiuhSimulation PUBLIC
    Qt6::Core Qt6::Concurrent Qt6::Gui Coin::Coin
    TonatiuhppKernel TonatiuhppLibraries
)

set(TONATIUHPP_M5_SCRIPTING_SOURCES
    script/DataObject.cpp
    script/NodeObject.cpp
    script/ScriptHost.cpp
    script/ScriptSceneServices.cpp
    script/ScriptSceneDocument.cpp
    script/ScriptInterceptionService.cpp
)
set(TONATIUHPP_M5_SCRIPTING_HEADERS
    script/DataObject.h
    script/NodeObject.h
    script/ScriptHost.h
    script/ScriptSceneServices.h
    script/ScriptSceneDocument.h
    script/ScriptInterceptionService.h
)
# M5.1B1: the shared QJSEngine host and DataObject file-IO need QtCore/Qml,
# not Coin, the kernel, the GUI executable, Widgets or SoQt.
# Both Q_OBJECT headers and their .cpp files move to the same static target.
add_library(TonatiuhScriptRuntime STATIC
    script/ScriptHost.cpp
    script/ScriptHost.h
    script/DataObject.cpp
    script/DataObject.h
)
target_include_directories(TonatiuhScriptRuntime PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)
target_link_libraries(TonatiuhScriptRuntime PUBLIC Qt6::Core Qt6::Qml)

# The scene-oriented facade continues to link the existing kernel and its
# legacy GUI dependencies. Only the minimal JS/file runtime is isolated here.
list(REMOVE_ITEM TONATIUHPP_M5_SCRIPTING_SOURCES
    script/ScriptHost.cpp script/DataObject.cpp)
list(REMOVE_ITEM TONATIUHPP_M5_SCRIPTING_HEADERS
    script/ScriptHost.h script/DataObject.h)

# QObject headers belong to this target alone: AUTOMOC must not
# generate a second copy in the executable after relocation.
add_library(TonatiuhScripting STATIC
    ${TONATIUHPP_M5_SCRIPTING_SOURCES}
    ${TONATIUHPP_M5_SCRIPTING_HEADERS}
)
# Shared GUI-script and CLI-script plumbing; only one compiled implementation.
# These sources contain no QtWidgets/SoQt includes or second physics kernel.
target_sources(TonatiuhScripting PRIVATE
    headless/HeadlessScriptHost.cpp headless/HeadlessScriptHost.h
    benchmark/BenchmarkRunner.cpp benchmark/BenchmarkRunner.h)
target_include_directories(TonatiuhScripting PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../libraries
    ${COIN3D_INCLUDE_DIR}
)
target_link_libraries(TonatiuhScripting PUBLIC
    Qt6::Core Qt6::Qml Qt6::Gui Coin::Coin
    TonatiuhScriptRuntime TonatiuhSimulation TonatiuhppKernel TonatiuhppLibraries
)

# Enforce the first boundary at configure time: these targets may not
# directly link QtWidgets/SoQt or directly include widget headers.
# This guard intentionally does NOT certify the transitive dependency graph.
function(tonatiuhpp_m5_assert_direct_service_boundary service)
    get_target_property(_service_links ${service} LINK_LIBRARIES)
    foreach(_forbidden IN ITEMS Qt6::Widgets SoQt::SoQt)
        if(_forbidden IN_LIST _service_links)
            message(FATAL_ERROR "M5 direct boundary violated: ${service} links ${_forbidden}")
        endif()
    endforeach()
    foreach(_source IN LISTS ARGN)
        file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/${_source}" _ui_includes
            REGEX "^[ \t]*#[ \t]*include[ \t]*[<\"](QApplication|QWidget|QDialog|QMessageBox|QFileDialog|Inventor/Qt/)")
        if(_ui_includes)
            message(FATAL_ERROR "M5 direct boundary violated in ${_source}: ${_ui_includes}")
        endif()
    endforeach()
endfunction()
tonatiuhpp_m5_assert_direct_service_boundary(TonatiuhSimulation
    ${TONATIUHPP_M5_SIMULATION_SOURCES} ${TONATIUHPP_M5_SIMULATION_HEADERS})
tonatiuhpp_m5_assert_direct_service_boundary(TonatiuhScripting
    ${TONATIUHPP_M5_SCRIPTING_SOURCES} ${TONATIUHPP_M5_SCRIPTING_HEADERS})

# M5.1B1: verify full CMake link-target closure for this independent runtime.
# In addition to scanning direct links, traverse imported transitive targets.
# Fail closed if any project kernel/GUI target, QtWidgets or SoQt reappears.
function(tonatiuhpp_m5_assert_runtime_closure)
    get_target_property(_runtime_direct TonatiuhScriptRuntime LINK_LIBRARIES)
    foreach(_direct IN LISTS _runtime_direct)
        if(NOT _direct STREQUAL "Qt6::Core" AND NOT _direct STREQUAL "Qt6::Qml")
            message(FATAL_ERROR "M5 runtime boundary: forbidden direct link: ${_direct}")
        endif()
    endforeach()
    set(_pending TonatiuhScriptRuntime)
    set(_visited)
    while(_pending)
        list(POP_FRONT _pending _target)
        if(_target IN_LIST _visited)
            continue()
        endif()
        list(APPEND _visited "${_target}")
        foreach(_property IN ITEMS LINK_LIBRARIES INTERFACE_LINK_LIBRARIES)
            get_target_property(_links "${_target}" "${_property}")
            if(NOT _links OR _links MATCHES "-NOTFOUND$")
                continue()
            endif()
            foreach(_forbidden IN ITEMS
                Qt6::Widgets SoQt::SoQt TonatiuhppKernel TonatiuhppLibraries
                TonatiuhSimulation TonatiuhScripting Coin::Coin)
                string(FIND "${_links}" "${_forbidden}" _found)
                if(NOT _found EQUAL -1)
                    message(FATAL_ERROR
                        "M5 runtime boundary: ${_target} transitively links ${_forbidden}")
                endif()
            endforeach()
            foreach(_linked IN LISTS _links)
                if(TARGET "${_linked}" AND NOT _linked IN_LIST _visited)
                    list(APPEND _pending "${_linked}")
                endif()
            endforeach()
        endforeach()
    endwhile()
    tonatiuhpp_m5_assert_direct_service_boundary(TonatiuhScriptRuntime
        script/ScriptHost.cpp script/ScriptHost.h
        script/DataObject.cpp script/DataObject.h)
    message(STATUS "M5.1B1: QtCore/Qml script runtime has no Tonatiuh kernel/GUI, Widgets or SoQt dependency")
endfunction()
tonatiuhpp_m5_assert_runtime_closure()

# M5.1 one physical simulation library and one kernel for BOTH adapters.
# The GUI presentation adapter is a separate QtWidgets/SoQt consumer.
target_sources(TonatiuhSimulation PRIVATE
    core/TonatiuhCore.cpp core/TonatiuhCore.h
    core/CorePluginRegistry.cpp core/CorePluginRegistry.h)
list(REMOVE_ITEM SOURCES core/TonatiuhCore.cpp core/CorePluginRegistry.cpp)
list(REMOVE_ITEM HEADERS core/TonatiuhCore.h core/CorePluginRegistry.h)

# The real CLI owns only QCoreApplication + CLI command dispatch. GUI
# widgets/SoQt and any test-only M5 smoke implementation are excluded.
add_executable(tonatiuhpp-cli
    headless/CliMain.cpp
    headless/HeadlessCommandRunner.cpp headless/HeadlessCommandRunner.h
    updater/IfwUpdateService.cpp updater/IfwUpdateService.h)
target_include_directories(tonatiuhpp-cli PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../libraries
    ${COIN3D_INCLUDE_DIR})
target_link_libraries(tonatiuhpp-cli PRIVATE
    TonatiuhScripting TonatiuhSimulation TonatiuhScriptRuntime
    TonatiuhppKernel TonatiuhppLibraries
    Qt6::Core Qt6::Gui Qt6::Qml Qt6::Concurrent Coin::Coin)
if(WIN32)
    target_link_libraries(tonatiuhpp-cli PRIVATE opengl32)
endif()
# P1: application executables use the top-level runtime policy.
target_compile_definitions(tonatiuhpp-cli PRIVATE
    APP_VERSION="${ProjectVersion}")

# Permanent executable name matches its CMake target.

# M5.2: the actual standalone QCoreApplication executable is part of
# the installed runtime, next to the ONE-kernel libraries and official plugins.
install(TARGETS tonatiuhpp-cli
    RUNTIME DESTINATION "${GLOBAL_INSTALL_BIN_DIR}")

# Full transitive CMake target graph protection for the *only* physics kernel,
# libraries and services. The separate GUI adapter may link Widgets/SoQt.
function(tonatiuhpp_m5_assert_one_kernel_link_closure root)
    set(_pending "${root}")
    set(_seen)
    while(_pending)
        list(POP_FRONT _pending _current)
        if(_current IN_LIST _seen)
            continue()
        endif()
        list(APPEND _seen "${_current}")
        if(NOT TARGET "${_current}")
            continue()
        endif()
        foreach(_property IN ITEMS LINK_LIBRARIES INTERFACE_LINK_LIBRARIES)
            get_target_property(_links "${_current}" "${_property}")
            if(NOT _links OR _links MATCHES "-NOTFOUND$")
                continue()
            endif()
            foreach(_forbidden IN ITEMS
                Qt6::Widgets Qt6::PrintSupport SoQt::SoQt
                TonatiuhppGuiPresentation TonatiuhHeadlessKernel
                TonatiuhHeadlessLibraries TonatiuhHeadlessSimulation
                TonatiuhHeadlessScripting tonatiuhpp-gui)
                string(FIND "${_links}" "${_forbidden}" _found)
                if(NOT _found EQUAL -1)
                    message(FATAL_ERROR "M5 one-kernel boundary: ${root} -> ${_current} -> ${_forbidden}")
                endif()
            endforeach()
            foreach(_linked IN LISTS _links)
                if(TARGET "${_linked}" AND NOT _linked IN_LIST _seen)
                    list(APPEND _pending "${_linked}")
                endif()
            endforeach()
        endforeach()
    endwhile()
    message(STATUS "M5 one-kernel dependency closure PASS: ${root}")
endfunction()

foreach(_m5_target IN ITEMS
    TonatiuhppLibraries TonatiuhppKernel TonatiuhScriptRuntime
    TonatiuhSimulation TonatiuhScripting tonatiuhpp-cli)
    tonatiuhpp_m5_assert_one_kernel_link_closure(${_m5_target})
endforeach()


# ----------------------------
# Executable
# ----------------------------
if(APPLE)
    add_executable(${ProjectName} MACOSX_BUNDLE ${SOURCES} ${HEADERS} ${FORMS} ${RESOURCES})
else()
    add_executable(${ProjectName} ${SOURCES} ${HEADERS} ${FORMS} ${RESOURCES})
endif()

if(WIN32)
    set(TONATIUHPP_APP_ICON "${CMAKE_CURRENT_SOURCE_DIR}/images/about/Tonatiuh.ico")
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/tonatiuhpp.rc.in"
        "${CMAKE_CURRENT_BINARY_DIR}/tonatiuhpp.rc"
        @ONLY
    )
    target_sources(${ProjectName} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/tonatiuhpp.rc")
endif()

# Keep on-disk name as "tonatiuhpp" except for the macOS app bundle.
if(APPLE)
    set_target_properties(${ProjectName} PROPERTIES
        OUTPUT_NAME "tonatiuhpp-gui"
        MACOSX_BUNDLE_BUNDLE_NAME "TonatiuhPP"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${ProjectVersion}"
        MACOSX_BUNDLE_BUNDLE_VERSION "${ProjectVersion}"
    )
else()
    set_target_properties(${ProjectName} PROPERTIES
        OUTPUT_NAME "tonatiuhpp-gui"
    )
endif()

# ----------------------------
# Include dirs for this target
# ----------------------------
target_include_directories(${ProjectName} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../libraries
    ${COIN3D_INCLUDE_DIR}
    ${SOQT_INCLUDE_DIR}
    $<TARGET_PROPERTY:TonatiuhppLibraries,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:TonatiuhppKernel,INTERFACE_INCLUDE_DIRECTORIES>
)

# ----------------------------
# Compile defs
# ----------------------------
target_compile_definitions(${ProjectName} PRIVATE APP_VERSION="${ProjectVersion}")

# ----------------------------
# Link libraries
# ----------------------------
target_link_libraries(${ProjectName}
    PRIVATE
        Coin::Coin
        SoQt::SoQt
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::PrintSupport
        Qt6::Qml
        Qt6::Concurrent
        TonatiuhScripting
        TonatiuhSimulation
        TonatiuhppLibraries
        TonatiuhppKernel
        TonatiuhppGuiPresentation
)

# Windows-specific OpenGL lib
if(WIN32)
    target_link_libraries(${ProjectName} PRIVATE opengl32)
endif()

# TONATIUHPP_P1_CENTRAL_WIN_RUNTIME: the shared build-tree stage
# and output policy are configured at source/CMakeLists.txt.

# ----------------------------
# Install rules
# ----------------------------
install(TARGETS ${ProjectName}
  BUNDLE DESTINATION "."
  RUNTIME DESTINATION "${GLOBAL_INSTALL_BIN_DIR}"
  LIBRARY DESTINATION "${GLOBAL_INSTALL_BIN_DIR}"
  ARCHIVE DESTINATION "${GLOBAL_INSTALL_BIN_DIR}"
)

# M5 native GUI acceptance: optional test-only Qt driver for REAL GUI dialogs.
# It is excluded from production builds configured with BUILD_TESTING=OFF.
# The helper is inert in other test builds without the explicit test env vars.
if(BUILD_TESTING)
    target_sources(tonatiuhpp-gui PRIVATE
        "${CMAKE_SOURCE_DIR}/../tests/integration/gui/NativeGuiAcceptance.cpp"
        "${CMAKE_SOURCE_DIR}/../tests/integration/gui/NativeGuiAcceptance.h")
    target_include_directories(tonatiuhpp-gui PRIVATE "${CMAKE_SOURCE_DIR}/../tests/integration/gui")
    target_compile_definitions(tonatiuhpp-gui PRIVATE TONATIUHPP_M5_NATIVE_GUI_TEST)
endif()
