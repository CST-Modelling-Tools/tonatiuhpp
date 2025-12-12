# ----------------------------
# 🔧 General Build Configuration
# ----------------------------
# Allow old packages (Coin, SoQt, etc.) that still request ancient CMake versions.
# This satisfies modern CMake by saying:
# "Our project works with policies from at least 3.5 and up."
if(NOT DEFINED CMAKE_POLICY_VERSION_MINIMUM)
  set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
endif()

cmake_minimum_required(VERSION 3.20) # 3.20+ gives modern features; raise if you need
project(TonatiuhPP VERSION 0.1.8.16 LANGUAGES CXX)

# Name of the main GUI target (used for install-time deployment; logical target name)
set(MAIN_APP_TARGET "TonatiuhPP" CACHE STRING "Main GUI executable target name")

# Options
option(TONATIUHPP_BUNDLE_RUNTIME "Bundle Qt/Coin runtime libs on install (Linux/Windows)" ON)

# On Linux, force it ON in the cache so presets / old caches can't accidentally disable it
if(UNIX AND NOT APPLE)
  set(TONATIUHPP_BUNDLE_RUNTIME ON CACHE BOOL "Bundle Qt/Coin runtime libs on install (Linux/Windows)" FORCE)
endif()

# Standard install dirs (no hard-coded 'bin', 'lib', etc.)
include(GNUInstallDirs)

# ------------------------------------------------------
# 🌍 Default install prefix (ALL platforms)
# ------------------------------------------------------
# Only change it when CMake was about to use its own default:
#  - /usr/local      on Unix-like
#  - C:/Program Files/<project> on Windows
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  if(DEFINED ENV{TonatiuhPPInstallDir} AND NOT "$ENV{TonatiuhPPInstallDir}" STREQUAL "")
    # 1) Explicit override from environment (works on all platforms)
    set(CMAKE_INSTALL_PREFIX "$ENV{TonatiuhPPInstallDir}" CACHE PATH "Install prefix" FORCE)
  elseif(WIN32)
    # 2) Windows: avoid C:\Program Files\ (admin rights) -> use %USERPROFILE%/tonatiuhpp
    if(DEFINED ENV{USERPROFILE} AND NOT "$ENV{USERPROFILE}" STREQUAL "")
      file(TO_CMAKE_PATH "$ENV{USERPROFILE}" _home)
      set(_default_prefix "${_home}/tonatiuhpp")
    else()
      # Fallback if USERPROFILE is somehow unset
      set(_default_prefix "C:/Users/Default/tonatiuhpp")
    endif()
    set(CMAKE_INSTALL_PREFIX "${_default_prefix}" CACHE PATH "Install prefix" FORCE)
  else()
    # 3) Unix-like (Linux, macOS, etc.): use $HOME/tonatiuhpp
    if(DEFINED ENV{HOME} AND NOT "$ENV{HOME}" STREQUAL "")
      set(CMAKE_INSTALL_PREFIX "$ENV{HOME}/tonatiuhpp" CACHE PATH "Install prefix" FORCE)
    else()
      # Very defensive fallback if HOME is missing
      set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/tonatiuhpp-install" CACHE PATH "Install prefix" FORCE)
    endif()
  endif()
endif()

message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")

# Build type for single-config generators
if(NOT DEFINED CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# ----------------------------
# 🔗 RPATH defaults (set BEFORE any targets are created)
# ----------------------------
if(UNIX AND NOT APPLE)
  # Make binaries look in the install bin dir and ../lib at runtime (ELF)
  set(CMAKE_INSTALL_RPATH "$ORIGIN;$ORIGIN/../${CMAKE_INSTALL_LIBDIR}")
  set(CMAKE_BUILD_RPATH   "$ORIGIN;$ORIGIN/../${CMAKE_INSTALL_LIBDIR}")
  set(CMAKE_BUILD_WITH_INSTALL_RPATH OFF)
  set(CMAKE_INSTALL_RPATH_USE_LINK_PATH OFF)
elseif(APPLE)
  # Enable modern macOS rpaths; we will extend them after Qt/Coin are located.
  set(CMAKE_MACOSX_RPATH ON)
  # Start with empty; we'll append later.
  set(CMAKE_BUILD_RPATH "")
  set(CMAKE_INSTALL_RPATH "")
  set(CMAKE_BUILD_WITH_INSTALL_RPATH OFF)
  set(CMAKE_INSTALL_RPATH_USE_LINK_PATH OFF)
endif()

# ----------------------------
# 🧭 Prefix bootstrap (no CLI args needed)
# ----------------------------
# 1) Add locally installed deps (from your third_party build)
list(PREPEND CMAKE_PREFIX_PATH "${CMAKE_CURRENT_LIST_DIR}/../third_party/_install")

# 2) Optionally include machine-local hints (auto-generated, gitignored)
set(_LOCAL_HINTS "${CMAKE_CURRENT_LIST_DIR}/../cmake/LocalDepsHints.cmake")
if(EXISTS "${_LOCAL_HINTS}")
  include("${_LOCAL_HINTS}")
endif()

# ----------------------------
# 🎯 Prefer config packages when available
# ----------------------------
set(CMAKE_FIND_PACKAGE_PREFER_CONFIG TRUE)

# If LocalDepsHints.cmake defined Qt6_DIR, backfill component dirs
if(DEFINED Qt6_DIR)
  get_filename_component(_qt_libcmake "${Qt6_DIR}" DIRECTORY)     # …/lib/cmake
  get_filename_component(_qt_lib     "${_qt_libcmake}" DIRECTORY) # …/lib
  get_filename_component(_qt_prefix  "${_qt_lib}" DIRECTORY)      # <QtPrefix>
  foreach(_qtcomp Core Gui Widgets OpenGL OpenGLWidgets PrintSupport Qml Concurrent DBus Network)
    if(NOT DEFINED Qt6${_qtcomp}_DIR)
      set(_cand "${_qt_prefix}/lib/cmake/Qt6${_qtcomp}")
      if(EXISTS "${_cand}/Qt6${_qtcomp}Config.cmake")
        set(Qt6${_qtcomp}_DIR "${_cand}" CACHE PATH "" FORCE)
      endif()
    endif()
  endforeach()
endif()

# Coin/SoQt: if not given, try from third_party/_install
foreach(_pkg Coin SoQt)
  if(NOT DEFINED ${_pkg}_DIR)
    file(GLOB _cand_dirs LIST_DIRECTORIES true
      "${CMAKE_CURRENT_LIST_DIR}/../third_party/_install/lib/cmake/${_pkg}-*")
    if(_cand_dirs)
      list(SORT _cand_dirs ORDER DESCENDING)
      list(GET _cand_dirs 0 _dir)
      if(EXISTS "${_dir}")
        set(${_pkg}_DIR "${_dir}" CACHE PATH "" FORCE)
      endif()
    endif()
  endif()
endforeach()

# ----------------------------
# 📚 Dependencies
# ----------------------------
find_package(Coin  REQUIRED CONFIG)
find_package(SoQt  REQUIRED CONFIG)
find_package(Qt6   REQUIRED COMPONENTS Core Gui Widgets PrintSupport Qml Concurrent DBus Network)

# --- Eigen3 (robust: config -> module -> header-only fallback) ---
# Allow user to point to headers
if(DEFINED ENV{EIGEN3_ROOT} AND NOT DEFINED EIGEN3_INCLUDE_DIR)
  set(EIGEN3_INCLUDE_DIR "$ENV{EIGEN3_ROOT}" CACHE PATH "Eigen3 include root (must contain Eigen/Core)")
endif()

find_package(Eigen3 QUIET CONFIG)
if(NOT Eigen3_FOUND)
  find_package(Eigen3 QUIET) # module mode
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
        "/usr/include/eigen3" "/usr/local/include/eigen3"
        "C:/eigen-3.4.1" "C:/eigen3"
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
# 🍏 macOS: Global RPATHs (Qt, SoQt/Coin, and own libs)
# ----------------------------
if(APPLE)
  # Base rpaths so the main exe and plugins can find our installed libs in <prefix>/lib
  set(_extra_rpaths
    "@executable_path/../${CMAKE_INSTALL_LIBDIR}"
    "@loader_path/../${CMAKE_INSTALL_LIBDIR}"
  )

  # Qt lib dir from Qt6::Core (we want .../macos/lib)
  get_target_property(_qt_core_location Qt6::Core LOCATION)
  if(_qt_core_location)
    # Example: /Applications/Qt/6.9.3/macos/lib/QtCore.framework/Versions/A/QtCore
    get_filename_component(_qt_fw_ver_dir "${_qt_core_location}" DIRECTORY)        # .../QtCore.framework/Versions/A
    get_filename_component(_qt_fw_dir     "${_qt_fw_ver_dir}/../.." REALPATH)      # .../QtCore.framework
    get_filename_component(_qt_lib_root   "${_qt_fw_dir}/.." REALPATH)             # .../macos/lib
    list(APPEND _extra_rpaths "${_qt_lib_root}")
    message(STATUS "macOS RPATH: adding Qt lib dir: ${_qt_lib_root}")
  endif()

  # SoQt/Coin libs from third_party/_install/lib
  set(_deps_lib_dir "${CMAKE_CURRENT_LIST_DIR}/../third_party/_install/${CMAKE_INSTALL_LIBDIR}")
  if(EXISTS "${_deps_lib_dir}")
    list(APPEND _extra_rpaths "${_deps_lib_dir}")
    message(STATUS "macOS RPATH: adding third_party lib dir: ${_deps_lib_dir}")
  endif()

  list(REMOVE_DUPLICATES _extra_rpaths)

  # Append to current RPATH lists (they may already contain values)
  list(APPEND CMAKE_BUILD_RPATH   ${_extra_rpaths})
  list(APPEND CMAKE_INSTALL_RPATH ${_extra_rpaths})

  set(CMAKE_BUILD_WITH_INSTALL_RPATH OFF)
  set(CMAKE_INSTALL_RPATH_USE_LINK_PATH OFF)

  message(STATUS "macOS final BUILD_RPATH:   ${CMAKE_BUILD_RPATH}")
  message(STATUS "macOS final INSTALL_RPATH: ${CMAKE_INSTALL_RPATH}")
endif()

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
# 🚚 Optional runtime bundling on Linux (OFF by default)
# ----------------------------
if(UNIX AND NOT APPLE AND TONATIUHPP_BUNDLE_RUNTIME)
  # Discover Qt lib & plugin dirs at configure time from imported targets
  set(_qt_lib_dir "")
  set(_qt_plugins_dir "")
  set(_qt_core_loc "")
  foreach(prop IMPORTED_LOCATION_RELEASE IMPORTED_LOCATION_DEBUG IMPORTED_LOCATION)
    if(NOT _qt_core_loc)
      get_target_property(_qt_core_loc Qt6::Core ${prop})
    endif()
  endforeach()
  if(_qt_core_loc)
    get_filename_component(_qt_lib_dir "${_qt_core_loc}" DIRECTORY)   # …/gcc_64/lib
    get_filename_component(_qt_prefix  "${_qt_lib_dir}" DIRECTORY)    # …/gcc_64
    set(_qt_plugins_dir "${_qt_prefix}/plugins")                      # …/gcc_64/plugins
  endif()

  # Allow explicit overrides (optional but useful for users)
  if(DEFINED TonatiuhQtLibDir AND EXISTS "${TonatiuhQtLibDir}")
    set(_qt_lib_dir "${TonatiuhQtLibDir}")
  endif()
  if(DEFINED TonatiuhQtPluginsDir AND EXISTS "${TonatiuhQtPluginsDir}")
    set(_qt_plugins_dir "${TonatiuhQtPluginsDir}")
  endif()

  # Our locally built third-party libs
  set(_tp_lib "${CMAKE_CURRENT_LIST_DIR}/../third_party/_install/${CMAKE_INSTALL_LIBDIR}")

  # Hand off to install-time script; inject values NOW via ${...}
  install(CODE
    "set(_installed_bindir \"${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}\")
     set(_installed_libdir \"${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}\")
     file(MAKE_DIRECTORY \"\${_installed_libdir}\")
     file(MAKE_DIRECTORY \"\${_installed_bindir}\")

     set(_qt_lib_dir     \"${_qt_lib_dir}\")
     set(_qt_plugins_dir \"${_qt_plugins_dir}\")
     set(_tp_lib         \"${_tp_lib}\")

     message(STATUS \"Bundling Qt/Coin runtime libs (Linux)\")

     # ---- Copy the exact Qt libs we link against (follow symlink chains) ----
     if(_qt_lib_dir AND EXISTS \"\${_qt_lib_dir}\")
       set(_qt_needed
         libQt6Core.so.6
         libQt6Gui.so.6
         libQt6Widgets.so.6
         libQt6PrintSupport.so.6
         libQt6OpenGL.so.6
         libQt6Qml.so.6
         libQt6Concurrent.so.6
         libQt6DBus.so.6
         libQt6Network.so.6
         libQt6XcbQpa.so.6
       )
       file(GLOB _icu_needed \"\${_qt_lib_dir}/libicu*.so*\")
       set(_qt_to_copy)
       foreach(name IN LISTS _qt_needed)
         if(EXISTS \"\${_qt_lib_dir}/\${name}\")
           list(APPEND _qt_to_copy \"\${_qt_lib_dir}/\${name}\")
         endif()
       endforeach()
       list(APPEND _qt_to_copy \${_icu_needed})
       if(_qt_to_copy)
         file(INSTALL
           DESTINATION \"\${_installed_libdir}\"
           TYPE FILE
           FOLLOW_SYMLINK_CHAIN
           FILES \${_qt_to_copy}
         )
       else()
         message(WARNING \"No Qt libs found to copy in: \${_qt_lib_dir}\")
       endif()
     else()
       message(STATUS \"Qt lib dir not set; skipping Qt lib copy\")
     endif()

     # ---- Copy Coin/SoQt/simage from our third_party prefix ----
     if(_tp_lib AND EXISTS \"\${_tp_lib}\")
       file(GLOB _tp_pick
         \"\${_tp_lib}/libCoin.so*\"
         \"\${_tp_lib}/libSoQt.so*\"
         \"\${_tp_lib}/libsimage.so*\"
       )
       if(_tp_pick)
         file(INSTALL
           DESTINATION \"\${_installed_libdir}\"
           TYPE FILE
           FOLLOW_SYMLINK_CHAIN
           FILES \${_tp_pick}
         )
       endif()
     endif()

     # ---- Copy essential Qt plugin folders into bin/ ----
     if(_qt_plugins_dir AND EXISTS \"\${_qt_plugins_dir}\")
       foreach(_subdir platforms xcbglintegrations imageformats)
         if(EXISTS \"\${_qt_plugins_dir}/\${_subdir}\")
           file(INSTALL
             DESTINATION \"\${_installed_bindir}\"
             TYPE DIRECTORY
             USE_SOURCE_PERMISSIONS
             FILES \"\${_qt_plugins_dir}/\${_subdir}\"
           )
         endif()
       endforeach()
       file(WRITE \"\${_installed_bindir}/qt.conf\" \"[Paths]\\nPlugins=.\\n\")
     else()
       message(STATUS \"Qt plugins directory not set; skipping plugin copy\")
     endif()
    ")
endif()

# ----------------------------
# 🚚 Windows runtime deployment
# ----------------------------
if(WIN32)
  # Put our third-party DLLs next to the exe on install (if you build them)
  install(DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/../third_party/_install/bin/"
          DESTINATION "${CMAKE_INSTALL_BINDIR}"
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
  if(WINDEPLOYQT_EXECUTABLE AND TONATIUHPP_BUNDLE_RUNTIME)
    # In multi-config generators, MAIN_APP_TARGET may be suffixed; use generator expression
    install(CODE
      "set(_installed_exe \"${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/${MAIN_APP_TARGET}$<IF:$<CONFIG:Debug>,d,>\")
       message(STATUS \"Running windeployqt on: \${_installed_exe}\")
       if(EXISTS \"\${_installed_exe}.exe\")
         set(_exe \"\${_installed_exe}.exe\")
       else()
         set(_exe \"\${_installed_exe}\")
       endif()
       execute_process(
         COMMAND \"${WINDEPLOYQT_EXECUTABLE}\" \"\${_exe}\"
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
  endif()

  # Export a small PATH helper for local debugging
  set(_thirdparty_bin "${CMAKE_CURRENT_LIST_DIR}/../third_party/_install/bin")
  set(_qt_bin_hint "${_qt_bin}")
  set(_env_file "${CMAKE_BINARY_DIR}/runtime_path_env.txt")
  file(WRITE "${_env_file}" "PATH=${_thirdparty_bin};${_qt_bin_hint};%PATH%\n")
  message(STATUS "Wrote runtime env file: ${_env_file}")
endif()

# ----------------------------
# 📁 Shared data install root
# ----------------------------
# Where non-binary data (examples, help, resources) will be installed.
# This will usually be:
#   <prefix>/share/tonatiuhpp
set(GLOBAL_INSTALL_DATA_DIR
    "${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME}"
    CACHE PATH "Install location for TonatiuhPP shared data"
)

# ----------------------------
# 📦 Install examples, help, resources (at install prefix root)
# ----------------------------
# Expected layout at the repository root:
#   <repo-root>/sources      (this CMakeLists.txt is here)
#   <repo-root>/examples
#   <repo-root>/help
#   <repo-root>/resources
#
# After install (with CMAKE_INSTALL_PREFIX=/home/USER/tonatiuhpp), we want:
#   /home/USER/tonatiuhpp/examples
#   /home/USER/tonatiuhpp/help
#   /home/USER/tonatiuhpp/resources

set(_EXAMPLES_SRC  "${CMAKE_CURRENT_LIST_DIR}/../examples")
set(_HELP_SRC      "${CMAKE_CURRENT_LIST_DIR}/../help")
set(_RESOURCES_SRC "${CMAKE_CURRENT_LIST_DIR}/../resources")

if(EXISTS "${_EXAMPLES_SRC}")
  install(
    DIRECTORY "${_EXAMPLES_SRC}/"
    DESTINATION "examples"
  )
else()
  message(STATUS "Examples directory not found: ${_EXAMPLES_SRC} (skipping install)")
endif()

if(EXISTS "${_HELP_SRC}")
  install(
    DIRECTORY "${_HELP_SRC}/"
    DESTINATION "help"
  )
else()
  message(STATUS "Help directory not found: ${_HELP_SRC} (skipping install)")
endif()

if(EXISTS "${_RESOURCES_SRC}")
  install(
    DIRECTORY "${_RESOURCES_SRC}/"
    DESTINATION "resources"
  )
else()
  message(STATUS "Resources directory not found: ${_RESOURCES_SRC} (skipping install)")
endif()

# ----------------------------
# 🏁 Linux launcher script
# ----------------------------
if(UNIX AND NOT APPLE)
  # Install the launcher script from <repo-root>/scripts
  install(
    PROGRAMS "${CMAKE_CURRENT_LIST_DIR}/../scripts/tonatiuhpp.sh"
    DESTINATION "${CMAKE_INSTALL_BINDIR}"
  )
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