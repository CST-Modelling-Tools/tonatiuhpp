# ----------------------------
# 🔧 General Build Configuration
# ----------------------------
cmake_minimum_required(VERSION 3.28)

# --- Compatibility shim for older package config files (e.g., Coin-4.0.1) ---
if(NOT DEFINED CMAKE_POLICY_VERSION_MINIMUM)
  set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
endif()
if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.27)
  set(CMAKE_POLICY_VERSION "3.10...${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}")
endif()

cmake_policy(SET CMP0074 NEW)  # Respect <Package>_ROOT variables if provided.

project(TonatiuhXX VERSION 0.1.8.16 LANGUAGES CXX)

# Name of the main GUI target (used for install-time deployment; logical target name)
set(MAIN_APP_TARGET "TonatiuhXX" CACHE STRING "Main GUI executable target name")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Only set CMAKE_BUILD_TYPE for single-config generators (e.g., Ninja, Makefiles)
if(NOT DEFINED CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# ----------------------------
# 🧭 Prefix bootstrap (no CLI args needed)
# ----------------------------
# 1) Always add our locally installed deps (from the Python script)
list(PREPEND CMAKE_PREFIX_PATH "${CMAKE_CURRENT_LIST_DIR}/../third_party/_install")

# 2) Optionally include machine-local hints (auto-generated, gitignored)
set(_LOCAL_HINTS "${CMAKE_CURRENT_LIST_DIR}/../cmake/LocalDepsHints.cmake")
if(EXISTS "${_LOCAL_HINTS}")
  include("${_LOCAL_HINTS}")
endif()

# ----------------------------
# 📚 Dependencies
# ----------------------------
find_package(Coin  REQUIRED CONFIG)
find_package(SoQt  REQUIRED CONFIG)
find_package(Qt6   REQUIRED COMPONENTS Core Gui Widgets PrintSupport Qml Concurrent)

# --- Eigen3 (robust: config -> module -> header-only fallback) ---
if(DEFINED ENV{EIGEN3_ROOT} AND NOT DEFINED EIGEN3_INCLUDE_DIR)
  set(EIGEN3_INCLUDE_DIR "$ENV{EIGEN3_ROOT}" CACHE PATH "Eigen3 include root (must contain Eigen/Core)")
endif()

find_package(Eigen3 QUIET CONFIG)
if(NOT Eigen3_FOUND)
  find_package(Eigen3 QUIET)
endif()

if(NOT Eigen3_FOUND)
  if(NOT EIGEN3_INCLUDE_DIR)
    find_path(EIGEN3_INCLUDE_DIR
      NAMES Eigen/Core
      HINTS
        $ENV{EIGEN3_ROOT}
        ${CMAKE_PREFIX_PATH}/include/eigen3
        ${CMAKE_PREFIX_PATH}/eigen3
      PATHS
        "C:/eigen-3.4.1"
        "C:/eigen3"
        "/usr/include/eigen3"
        "/usr/local/include/eigen3"
    )
  endif()

  if(NOT EIGEN3_INCLUDE_DIR)
    message(FATAL_ERROR
      "Eigen3 headers not found.\n"
      "Place Eigen in a standard location or set EIGEN3_INCLUDE_DIR to the folder containing Eigen/Core.")
  endif()

  add_library(Eigen3::Eigen INTERFACE IMPORTED)
  set_target_properties(Eigen3::Eigen PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${EIGEN3_INCLUDE_DIR}"
  )
  set(Eigen3_FOUND TRUE)
endif()

# For informative status output
get_target_property(COIN3D_INCLUDE_DIR Coin::Coin INTERFACE_INCLUDE_DIRECTORIES)
get_target_property(SOQT_INCLUDE_DIR   SoQt::SoQt   INTERFACE_INCLUDE_DIRECTORIES)

# ----------------------------
# 🛠️ Compiler Definitions
# ----------------------------
if(WIN32)
  add_compile_definitions(SOQT_DLL)
else()
  add_compile_definitions(SOQT_NOT_DLL)
endif()
add_compile_definitions(QT_NO_DEPRECATED_WARNINGS)

# ----------------------------
# 📂 Add Subdirectories
# ----------------------------
add_subdirectory(libraries)
add_subdirectory(kernel)
add_subdirectory(SunPath)
add_subdirectory(application)
add_subdirectory(plugins)

# ----------------------------
# 🪄 Windows runtime helper (definition only; call it in application/)
# ----------------------------
function(tonatiuh_setup_windows_runtime target_name)
  if(NOT WIN32 OR NOT TARGET "${target_name}")
    return()
  endif()

  # Where SoQt/Coin were installed by build_deps.py
  set(_thirdparty_bin "${CMAKE_CURRENT_LIST_DIR}/../third_party/_install/bin")

  # Try to discover Qt bin dir from imported Qt targets
  set(_qt_bin "")
  get_target_property(_qt_qmake Qt6::qmake IMPORTED_LOCATION)
  if(_qt_qmake)
    get_filename_component(_qt_bin "${_qt_qmake}" DIRECTORY)
  else()
    get_target_property(_qt_core_loc Qt6::Core IMPORTED_LOCATION)
    if(_qt_core_loc)
      get_filename_component(_qt_bin "${_qt_core_loc}" DIRECTORY)
    endif()
  endif()

  # 1) Copy SoQt/Coin (and any other third_party) DLLs next to the built exe
  add_custom_command(TARGET "${target_name}" POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E echo "Copying third_party DLLs into build output…"
    COMMAND "${CMAKE_COMMAND}" -E copy_directory
            "${_thirdparty_bin}"
            "$<TARGET_FILE_DIR:${target_name}>"
    VERBATIM)

  # 2) Run windeployqt on the built exe to bring Qt DLLs & plugins into the build dir
  if(_qt_bin)
    find_program(WINDEPLOYQT_EXECUTABLE NAMES windeployqt HINTS "${_qt_bin}")
  endif()
  if(WINDEPLOYQT_EXECUTABLE)
    add_custom_command(TARGET "${target_name}" POST_BUILD
      COMMAND "${CMAKE_COMMAND}" -E echo "Running windeployqt for $<TARGET_FILE:${target_name}>…"
      COMMAND "${WINDEPLOYQT_EXECUTABLE}" "$<TARGET_FILE:${target_name}>"
              --no-translations --compiler-runtime --verbose 1
      VERBATIM)
  else()
    message(WARNING
      "windeployqt not found; build-tree runs may miss Qt DLLs/plugins. "
      "Qt bin not detected: '${_qt_bin}'")
  endif()

  # 3) Make F5/Debugging in Visual Studio/VS Code inherit correct PATH
  if(MSVC)
    set(_dbg_path "PATH=${_qt_bin};${_thirdparty_bin};%PATH%")
    set_property(TARGET "${target_name}" PROPERTY VS_DEBUGGER_ENVIRONMENT "${_dbg_path}")
  endif()
endfunction()

# NOTE: Do NOT call tonatiuh_setup_windows_runtime() here. Call it
#       inside application/CMakeLists.txt AFTER add_executable().

# ----------------------------
# 📦 Installation
# ----------------------------
if(DEFINED ENV{TonatiuhXXInstallDir})
  set(CMAKE_INSTALL_PREFIX "$ENV{TonatiuhXXInstallDir}" CACHE PATH "Installation directory" FORCE)
elseif(WIN32)
  set(CMAKE_INSTALL_PREFIX "C:/TonatiuhXX" CACHE PATH "Installation directory" FORCE)
else()
  set(CMAKE_INSTALL_PREFIX "/opt/TonatiuhXX" CACHE PATH "Installation directory" FORCE)
endif()

message(STATUS "Installation directory: ${CMAKE_INSTALL_PREFIX}")

set(GLOBAL_INSTALL_BIN_DIR       "${CMAKE_INSTALL_PREFIX}/bin")
set(GLOBAL_INSTALL_INCLUDE_DIR   "${CMAKE_INSTALL_PREFIX}/include")
set(GLOBAL_INSTALL_EXAMPLES_DIR  "${CMAKE_INSTALL_PREFIX}/examples")
set(GLOBAL_INSTALL_HELP_DIR      "${CMAKE_INSTALL_PREFIX}/help")
set(GLOBAL_INSTALL_RESOURCES_DIR "${CMAKE_INSTALL_PREFIX}/resources")

# Install the main binary if a target named after the project exists (harmless if not).
if(TARGET ${PROJECT_NAME})
  install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION "${GLOBAL_INSTALL_BIN_DIR}"
    LIBRARY DESTINATION "${GLOBAL_INSTALL_BIN_DIR}"
    ARCHIVE DESTINATION "${GLOBAL_INSTALL_BIN_DIR}"
  )
endif()

# Optional resource installs (only if dirs exist one level above source)
if(EXISTS "${CMAKE_SOURCE_DIR}/../examples")
  install(DIRECTORY ${CMAKE_SOURCE_DIR}/../examples/
    DESTINATION "${GLOBAL_INSTALL_EXAMPLES_DIR}"
    FILES_MATCHING PATTERN "*.*"
  )
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/../help")
  install(DIRECTORY ${CMAKE_SOURCE_DIR}/../help/
    DESTINATION "${GLOBAL_INSTALL_HELP_DIR}"
    FILES_MATCHING PATTERN "*.*"
  )
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/../resources")
  install(DIRECTORY ${CMAKE_SOURCE_DIR}/../resources/
    DESTINATION "${GLOBAL_INSTALL_RESOURCES_DIR}"
    FILES_MATCHING PATTERN "*.*"
  )
endif()

# ----------------------------
# 🚚 Install-time deployment (Windows)
# ----------------------------
# RPATH for Unix/macOS
if(UNIX AND NOT APPLE)
  set(CMAKE_INSTALL_RPATH "$ORIGIN")
  set(CMAKE_BUILD_WITH_INSTALL_RPATH OFF)
endif()
if(APPLE)
  set(CMAKE_INSTALL_RPATH "@loader_path")
  set(CMAKE_BUILD_WITH_INSTALL_RPATH OFF)
endif()

# Windows: copy Coin/SoQt DLLs and run windeployqt at install time
if(WIN32)
  install(DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/../third_party/_install/bin/"
          DESTINATION "${GLOBAL_INSTALL_BIN_DIR}"
          FILES_MATCHING PATTERN "*.dll")

  # Try to locate Qt bin dir via imported Qt targets.
  set(_qt_bin "")
  get_target_property(_qt_qmake Qt6::qmake IMPORTED_LOCATION)
  if(_qt_qmake)
    get_filename_component(_qt_bin "${_qt_qmake}" DIRECTORY)
  else()
    get_target_property(_qt_core_loc Qt6::Core IMPORTED_LOCATION)
    if(_qt_core_loc)
      get_filename_component(_qt_bin "${_qt_core_loc}" DIRECTORY)
    endif()
  endif()

  find_program(WINDEPLOYQT_EXECUTABLE NAMES windeployqt HINTS "${_qt_bin}")
  if(WINDEPLOYQT_EXECUTABLE)
    set(_installed_exe "${GLOBAL_INSTALL_BIN_DIR}/$<TARGET_FILE_NAME:${MAIN_APP_TARGET}>")
    install(CODE
      "message(STATUS \"Running windeployqt on: ${_installed_exe}\")
       execute_process(
         COMMAND \"${WINDEPLOYQT_EXECUTABLE}\" \"${_installed_exe}\"
                 --no-translations --compiler-runtime --verbose 1
         RESULT_VARIABLE _wdq_rv
         OUTPUT_VARIABLE _wdq_out
         ERROR_VARIABLE  _wdq_err
       )
       if(NOT _wdq_rv EQUAL 0)
         message(WARNING \"windeployqt returned code \${_wdq_rv}\")
         message(STATUS \"windeployqt stdout: \${_wdq_out}\")
         message(STATUS \"windeployqt stderr: \${_wdq_err}\")
       endif()"
    )
  else()
    message(WARNING "windeployqt not found; Qt plugins may be missing at runtime.")
  endif()
endif()

# ----------------------------
# ✅ Final Status
# ----------------------------
set(_EIGEN_PRINT "<unknown>")
if(DEFINED EIGEN3_INCLUDE_DIR)
  set(_EIGEN_PRINT "${EIGEN3_INCLUDE_DIR}")
else()
  get_target_property(_EIGEN_PRINT Eigen3::Eigen INTERFACE_INCLUDE_DIRECTORIES)
endif()

message(STATUS "CMake configuration completed successfully!")
message(STATUS "Dependencies:")
message(STATUS "  Eigen3: ${_EIGEN_PRINT}")
message(STATUS "  Coin3D: ${COIN3D_INCLUDE_DIR}")
message(STATUS "  SoQt:   ${SOQT_INCLUDE_DIR}")