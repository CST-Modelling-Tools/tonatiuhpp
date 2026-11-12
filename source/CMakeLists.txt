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
project(TonatiuhPP VERSION 0.1.8.26 LANGUAGES CXX)

# Name of the main GUI target (used for install-time deployment; logical target name)
set(MAIN_APP_TARGET "tonatiuhpp-gui")

# Options
option(TONATIUHPP_BUNDLE_RUNTIME "Bundle Qt/Coin runtime libs on install (Linux/Windows)" ON)
set(TONATIUHPP_TEST_EXECUTABLE "" CACHE FILEPATH "Optional executable for installed-runtime headless CTest smoke tests; default is the build-tree target")

include(CTest)

# On Linux, force it ON in the cache so presets / old caches can't accidentally disable it
if(UNIX AND NOT APPLE)
  set(TONATIUHPP_BUNDLE_RUNTIME ON CACHE BOOL "Bundle Qt/Coin runtime libs on install (Linux/Windows)" FORCE)
endif()

# Standard install dirs (no hard-coded 'bin', 'lib', etc.)
include(GNUInstallDirs)

# Keep legacy subdirectory install rules aligned with GNUInstallDirs.
set(GLOBAL_INSTALL_BIN_DIR
    "${CMAKE_INSTALL_BINDIR}"
    CACHE PATH "Install location for TonatiuhPP binaries"
)

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

# TONATIUHPP_P1_CENTRAL_WIN_RUNTIME: output policy
# Windows executables and *internal* shared DLLs share one directory per
# configuration for BOTH MSVC multi-config and Ninja single-config builds.
# The generator expression prevents an extra /Release suffix on MSVC.
if(WIN32)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>")
endif()


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

if(APPLE)
  find_library(TONATIUHPP_AGL_FRAMEWORK AGL)

  function(tonatiuh_macos_value_has_agl output_variable)
    set(_value "${ARGN}")
    if(_value MATCHES "(AGL\\.framework|(^|[^A-Za-z0-9_])AGL([^A-Za-z0-9_]|$)|-framework[ =]+AGL|-Wl,-framework,AGL|LINKER:-framework,AGL)")
      set(${output_variable} TRUE PARENT_SCOPE)
    else()
      set(${output_variable} FALSE PARENT_SCOPE)
    endif()
  endfunction()

  function(tonatiuh_remove_macos_agl_framework output_variable)
    set(_cleaned)
    set(_pending_framework FALSE)

    foreach(_item IN LISTS ARGN)
      if(_pending_framework)
        if(NOT _item STREQUAL "AGL")
          list(APPEND _cleaned "-framework" "${_item}")
        endif()
        set(_pending_framework FALSE)
        continue()
      endif()

      if(_item STREQUAL "-framework")
        set(_pending_framework TRUE)
        continue()
      endif()

      if(_item STREQUAL "AGL"
          OR _item MATCHES "^\"?((LINKER:)?SHELL:)?-framework[ =]+AGL\"?$"
          OR _item MATCHES "^\"?(LINKER:)?-Wl,-framework,AGL\"?$"
          OR _item MATCHES "^\"?LINKER:-framework,AGL\"?$"
          OR _item MATCHES "(^|/)AGL\\.framework($|/)")
        continue()
      endif()

      set(_cleaned_item "${_item}")
      string(REGEX REPLACE "\"?((LINKER:)?SHELL:)?-framework[ =]+AGL\"?" "" _cleaned_item "${_cleaned_item}")
      string(REGEX REPLACE "\"?(LINKER:)?-Wl,-framework,AGL\"?" "" _cleaned_item "${_cleaned_item}")
      string(REGEX REPLACE "\"?LINKER:-framework,AGL\"?" "" _cleaned_item "${_cleaned_item}")
      string(REGEX REPLACE "\"?([^ ;\"]*/)?AGL\\.framework(/[^ ;\"]*)?\"?" "" _cleaned_item "${_cleaned_item}")
      string(STRIP "${_cleaned_item}" _cleaned_item)
      if(NOT _cleaned_item STREQUAL "")
        list(APPEND _cleaned "${_cleaned_item}")
      endif()
    endforeach()

    if(_pending_framework)
      list(APPEND _cleaned "-framework")
    endif()

    set(${output_variable} "${_cleaned}" PARENT_SCOPE)
  endfunction()

  function(tonatiuh_macos_agl_target_properties output_variable target_name)
    set(_properties
      INTERFACE_LINK_LIBRARIES
      INTERFACE_LINK_OPTIONS
      LINK_LIBRARIES
      LINK_OPTIONS
      IMPORTED_LINK_INTERFACE_LIBRARIES
      IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE
      IMPORTED_LINK_INTERFACE_LIBRARIES_RELWITHDEBINFO
      IMPORTED_LINK_OPTIONS
      IMPORTED_LINK_OPTIONS_RELEASE
      IMPORTED_LINK_OPTIONS_RELWITHDEBINFO
    )

    if(TARGET "${target_name}")
      get_property(_configs TARGET "${target_name}" PROPERTY IMPORTED_CONFIGURATIONS)
      foreach(_config IN LISTS _configs)
        string(TOUPPER "${_config}" _config_upper)
        list(APPEND _properties
          "IMPORTED_LINK_INTERFACE_LIBRARIES_${_config_upper}"
          "IMPORTED_LINK_OPTIONS_${_config_upper}"
        )
      endforeach()
    endif()

    list(REMOVE_DUPLICATES _properties)
    set(${output_variable} "${_properties}" PARENT_SCOPE)
  endfunction()

  function(tonatiuh_scrub_macos_agl_from_target target_name)
    if(ARGC GREATER 1)
      set(_stage "${ARGV1}")
    else()
      set(_stage "target scan")
    endif()

    if(NOT TARGET "${target_name}")
      return()
    endif()

    tonatiuh_macos_agl_target_properties(_properties "${target_name}")

    foreach(_property IN LISTS _properties)
      get_property(_property_is_set TARGET "${target_name}" PROPERTY "${_property}" SET)
      if(NOT _property_is_set)
        continue()
      endif()

      get_target_property(_value "${target_name}" "${_property}")
      if(NOT _value)
        continue()
      endif()

      tonatiuh_macos_value_has_agl(_has_agl "${_value}")
      if(NOT _has_agl)
        continue()
      endif()

      message(STATUS "macOS AGL diagnostic [${_stage}] target ${target_name} ${_property} before sanitize: ${_value}")
      tonatiuh_remove_macos_agl_framework(_cleaned "${_value}")
      if(NOT "${_cleaned}" STREQUAL "${_value}")
        set_property(TARGET "${target_name}" PROPERTY "${_property}" "${_cleaned}")
      endif()

      get_target_property(_after_value "${target_name}" "${_property}")
      tonatiuh_macos_value_has_agl(_after_has_agl "${_after_value}")
      if(_after_has_agl)
        message(STATUS "macOS AGL diagnostic [${_stage}] target ${target_name} ${_property} after sanitize: ${_after_value}")
      else()
        message(STATUS "macOS AGL diagnostic [${_stage}] target ${target_name} ${_property} after sanitize: <removed>")
      endif()
    endforeach()
  endfunction()

  function(tonatiuh_collect_macos_known_targets output_variable)
    set(_targets)
    set(_pending_dirs "${CMAKE_SOURCE_DIR}")

    while(_pending_dirs)
      list(POP_FRONT _pending_dirs _dir)

      get_property(_build_targets DIRECTORY "${_dir}" PROPERTY BUILDSYSTEM_TARGETS)
      get_property(_imported_targets DIRECTORY "${_dir}" PROPERTY IMPORTED_TARGETS)
      list(APPEND _targets ${_build_targets} ${_imported_targets})

      get_property(_subdirs DIRECTORY "${_dir}" PROPERTY SUBDIRECTORIES)
      list(APPEND _pending_dirs ${_subdirs})
    endwhile()

    if(_targets)
      list(REMOVE_DUPLICATES _targets)
    endif()
    set(${output_variable} "${_targets}" PARENT_SCOPE)
  endfunction()

  function(tonatiuh_scrub_macos_agl_from_known_targets stage)
    if(TONATIUHPP_AGL_FRAMEWORK)
      return()
    endif()

    tonatiuh_collect_macos_known_targets(_targets)
    foreach(_target IN LISTS _targets)
      tonatiuh_scrub_macos_agl_from_target("${_target}" "${stage}")
    endforeach()
  endfunction()

  function(tonatiuh_scrub_macos_agl_from_variables stage)
    if(TONATIUHPP_AGL_FRAMEWORK)
      return()
    endif()

    foreach(_variable
        OPENGL_LIBRARIES
        COIN_LIBRARIES
        SOQT_LIBRARIES
        CMAKE_EXE_LINKER_FLAGS
        CMAKE_SHARED_LINKER_FLAGS
        CMAKE_MODULE_LINKER_FLAGS
        OPENGL_gl_LIBRARY
        OPENGL_LIBRARY
        Coin_LIBRARIES
        SoQt_LIBRARIES)
      if(DEFINED ${_variable})
        set(_value "${${_variable}}")
        tonatiuh_macos_value_has_agl(_has_agl "${_value}")
        if(NOT _has_agl)
          continue()
        endif()

        message(STATUS "macOS AGL diagnostic [${stage}] variable ${_variable} before sanitize: ${_value}")
        tonatiuh_remove_macos_agl_framework(_cleaned "${_value}")
        set(${_variable} "${_cleaned}" PARENT_SCOPE)

        tonatiuh_macos_value_has_agl(_after_has_agl "${_cleaned}")
        if(_after_has_agl)
          message(STATUS "macOS AGL diagnostic [${stage}] variable ${_variable} after sanitize: ${_cleaned}")
        else()
          message(STATUS "macOS AGL diagnostic [${stage}] variable ${_variable} after sanitize: <removed>")
        endif()
      endif()
    endforeach()
  endfunction()

  function(tonatiuh_fail_if_macos_agl_in_target_link_properties target_name stage)
    if(TONATIUHPP_AGL_FRAMEWORK OR NOT TARGET "${target_name}")
      return()
    endif()

    set(_failed FALSE)
    foreach(_property LINK_LIBRARIES LINK_OPTIONS)
      get_property(_property_is_set TARGET "${target_name}" PROPERTY "${_property}" SET)
      if(NOT _property_is_set)
        continue()
      endif()

      get_target_property(_value "${target_name}" "${_property}")
      tonatiuh_macos_value_has_agl(_has_agl "${_value}")
      if(_has_agl)
        message(STATUS "macOS AGL diagnostic [${stage}] target ${target_name} ${_property} still contains AGL: ${_value}")
        set(_failed TRUE)
      endif()
    endforeach()

    if(_failed)
      message(FATAL_ERROR "AGL remains in ${target_name} link properties after macOS sanitizer.")
    endif()
  endfunction()

  if(NOT TONATIUHPP_AGL_FRAMEWORK)
    tonatiuh_scrub_macos_agl_from_known_targets("dependency import")
    tonatiuh_scrub_macos_agl_from_variables("dependency import")
  endif()
endif()

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
if(APPLE AND COMMAND tonatiuh_scrub_macos_agl_from_known_targets)
  tonatiuh_scrub_macos_agl_from_known_targets("SunPath post-link")
  tonatiuh_scrub_macos_agl_from_variables("SunPath post-link")
  tonatiuh_fail_if_macos_agl_in_target_link_properties(SunPath "SunPath post-link")
endif()
add_subdirectory(application)
add_subdirectory(plugins)

file(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/p2-artifacts-$<CONFIG>.txt"
  CONTENT "GUI=$<TARGET_FILE:tonatiuhpp-gui>\nCLI=$<TARGET_FILE:tonatiuhpp-cli>\nKERNEL=$<TARGET_FILE:TonatiuhppKernel>\nLIBRARIES=$<TARGET_FILE:TonatiuhppLibraries>\nPRESENTATION=$<TARGET_FILE:TonatiuhppGuiPresentation>\n")

# M5.1 official scene + File exporter plugin links must be compatible with the
# same QtWidgets/SoQt-free TonatiuhppKernel as the QCoreApplication CLI.
# This runs AFTER add_subdirectory(plugins), when plugin targets exist.
foreach(_m5_plugin IN ITEMS
    AirMirval MaterialSpecular PhotonsFile
    ShapeElliptic ShapeFunctionXYZ ShapeFunctionZ ShapeHyperbolic ShapeMapN ShapeMesh
    SunBuie SunGaussian)
  if(NOT TARGET ${_m5_plugin})
    message(FATAL_ERROR "M5 one-kernel: expected official plugin ${_m5_plugin} missing")
  endif()
  tonatiuhpp_m5_assert_one_kernel_link_closure(${_m5_plugin})
endforeach()

if(BUILD_TESTING)
  add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../tests" "${CMAKE_BINARY_DIR}/tests")
endif()

# TONATIUHPP_P1_CENTRAL_WIN_RUNTIME: build-tree-only dependency deployment
# Stage external Qt/Coin/SoQt DLLs once per configuration BEFORE any executable
# builds. Never copy Tonatiuh++ internal DLLs: their targets emit them directly
# to build/bin/<config>. This always-running dependency also fixes incremental
# builds that previously skipped an executable's POST_BUILD copy hook.
if(WIN32)
  set(_tonatiuhpp_p1_executables tonatiuhpp-gui tonatiuhpp-cli)
  if(BUILD_TESTING)
    list(APPEND _tonatiuhpp_p1_executables
      tonatiuhpp_scientific_equivalence_tests
      tonatiuhpp_math_tests tonatiuhpp_math_box2d_tests
      tonatiuhpp_standard_random_tests)
  endif()

  set(_tonatiuhpp_p1_runtime_dll_expressions)
  foreach(_tonatiuhpp_p1_target IN LISTS _tonatiuhpp_p1_executables)
    if(NOT TARGET ${_tonatiuhpp_p1_target})
      message(FATAL_ERROR "P1 expected target missing: ${_tonatiuhpp_p1_target}")
    endif()
    list(APPEND _tonatiuhpp_p1_runtime_dll_expressions
      "$<TARGET_RUNTIME_DLLS:${_tonatiuhpp_p1_target}>")
  endforeach()
  # Guarantee core Qt modules even if third-party imported Coin/SoQt targets
  # do not describe every dependent binary in their imported target metadata.
  foreach(_tonatiuhpp_p1_module IN ITEMS Core Gui Widgets Qml Concurrent
                                           PrintSupport DBus Network)
    if(TARGET Qt6::${_tonatiuhpp_p1_module})
      list(APPEND _tonatiuhpp_p1_runtime_dll_expressions
        "$<TARGET_FILE:Qt6::${_tonatiuhpp_p1_module}>")
    endif()
  endforeach()

  file(GENERATE
    OUTPUT "${CMAKE_BINARY_DIR}/tonatiuhpp-stage-win-$<CONFIG>.cmake"
    CONTENT "set(TONATIUHPP_P1_RUNTIME_DIR [==[${CMAKE_BINARY_DIR}/bin/$<CONFIG>]==])\nset(TONATIUHPP_P1_QT_CORE_DLL [==[$<TARGET_FILE:Qt6::Core>]==])\nset(TONATIUHPP_P1_THIRDPARTY_BIN [==[${CMAKE_CURRENT_LIST_DIR}/../third_party/_install/bin]==])\nset(TONATIUHPP_P1_DLLS [==[${_tonatiuhpp_p1_runtime_dll_expressions}]==])\ninclude([==[${CMAKE_CURRENT_LIST_DIR}/cmake/StageWindowsRuntime.cmake]==])\n")

  add_custom_target(tonatiuhpp_stage_windows_runtime
    COMMAND "${CMAKE_COMMAND}" -P
            "${CMAKE_BINARY_DIR}/tonatiuhpp-stage-win-$<CONFIG>.cmake"
    COMMENT "Stage common Windows Qt/Coin/SoQt DLLs and Qt plugins once"
    VERBATIM)
  foreach(_tonatiuhpp_p1_target IN LISTS _tonatiuhpp_p1_executables)
    add_dependencies(${_tonatiuhpp_p1_target} tonatiuhpp_stage_windows_runtime)
  endforeach()
endif()


# ----------------------------
# 🚚 Optional runtime bundling on Linux (ON by default)
# ----------------------------
if(UNIX AND NOT APPLE AND TONATIUHPP_BUNDLE_RUNTIME)
  # Our locally built third-party libs
  set(_tp_lib "${CMAKE_CURRENT_LIST_DIR}/../third_party/_install/${CMAKE_INSTALL_LIBDIR}")

  set(_qt_core_lib "")
  foreach(_qt_core_prop IMPORTED_LOCATION_RELEASE IMPORTED_LOCATION_RELWITHDEBINFO IMPORTED_LOCATION_MINSIZEREL IMPORTED_LOCATION)
    if(NOT _qt_core_lib)
      get_target_property(_qt_core_candidate Qt6::Core ${_qt_core_prop})
      if(_qt_core_candidate AND NOT _qt_core_candidate MATCHES "-NOTFOUND$")
        set(_qt_core_lib "${_qt_core_candidate}")
      endif()
    endif()
  endforeach()
  if(NOT _qt_core_lib)
    get_target_property(_qt_core_lib Qt6::Core LOCATION)
  endif()
  if(NOT _qt_core_lib OR _qt_core_lib MATCHES "-NOTFOUND$")
    message(FATAL_ERROR "Could not resolve Qt6::Core library location for Linux runtime bundling.")
  endif()

  get_filename_component(_qt_lib_dir "${_qt_core_lib}" DIRECTORY)
  get_filename_component(_qt_prefix "${_qt_lib_dir}/.." REALPATH)

  set(_qt_plugin_dir "")
  foreach(_qt_plugin_candidate
      "${_qt_prefix}/plugins"
      "${_qt_lib_dir}/qt6/plugins"
      "${_qt_lib_dir}/plugins"
      "${_qt_prefix}/${CMAKE_INSTALL_LIBDIR}/qt6/plugins"
      "${_qt_prefix}/${CMAKE_INSTALL_LIBDIR}/plugins")
    if(NOT _qt_plugin_dir AND EXISTS "${_qt_plugin_candidate}/platforms")
      set(_qt_plugin_dir "${_qt_plugin_candidate}")
    endif()
  endforeach()
  if(NOT _qt_plugin_dir)
    message(FATAL_ERROR "Could not locate Qt platform plugins under Qt prefix: ${_qt_prefix}")
  endif()

  set(_qt_version "${Qt6_VERSION}")
  if(NOT _qt_version)
    set(_qt_version "${Qt6Core_VERSION}")
  endif()

  # Hand off to install-time script; inject values NOW via ${...}
  install(CODE
    "set(_installed_bindir \"\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}\")
     set(_installed_libdir \"\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}\")
     file(MAKE_DIRECTORY \"\${_installed_libdir}\")
     file(MAKE_DIRECTORY \"\${_installed_bindir}\")

     set(_tp_lib \"${_tp_lib}\")
     set(_qt_lib_dir \"${_qt_lib_dir}\")
     set(_qt_plugin_dir \"${_qt_plugin_dir}\")
     set(_qt_version \"${_qt_version}\")

     file(REMOVE \"\${_installed_bindir}/qt.conf\")
     foreach(_subdir
         platforms xcbglintegrations imageformats tls iconengines platformthemes styles
         generic networkinformation egldeviceintegrations wayland-decoration-client
         wayland-graphics-integration-client wayland-shell-integration sqldrivers)
       file(REMOVE_RECURSE \"\${_installed_bindir}/\${_subdir}\")
     endforeach()
     file(GLOB _stale_qt_runtime
       \"\${_installed_libdir}/libQt6*.so*\"
       \"\${_installed_libdir}/libicu*.so*\"
     )
     if(_stale_qt_runtime)
       file(REMOVE \${_stale_qt_runtime})
     endif()

     message(STATUS \"Bundling Qt runtime libs/plugins (Linux) from \${_qt_lib_dir} / \${_qt_plugin_dir}\")

     file(GLOB _qt_runtime_libs
       \"\${_qt_lib_dir}/libQt6*.so*\"
       \"\${_qt_lib_dir}/libicu*.so*\"
     )
     if(NOT _qt_runtime_libs)
       message(FATAL_ERROR \"No Qt runtime libraries found in \${_qt_lib_dir}\")
     endif()
     file(INSTALL
       DESTINATION \"\${_installed_libdir}\"
       TYPE FILE
       FOLLOW_SYMLINK_CHAIN
       FILES \${_qt_runtime_libs}
     )

     file(GLOB _qt_plugin_subdirs LIST_DIRECTORIES true \"\${_qt_plugin_dir}/*\")
     if(NOT _qt_plugin_subdirs)
       message(FATAL_ERROR \"No Qt plugin directories found in \${_qt_plugin_dir}\")
     endif()
     foreach(_qt_plugin_subdir IN LISTS _qt_plugin_subdirs)
       if(IS_DIRECTORY \"\${_qt_plugin_subdir}\")
         get_filename_component(_qt_plugin_name \"\${_qt_plugin_subdir}\" NAME)
         file(REMOVE_RECURSE \"\${_installed_bindir}/\${_qt_plugin_name}\")
         file(INSTALL
           DESTINATION \"\${_installed_bindir}\"
           TYPE DIRECTORY
           FILES \"\${_qt_plugin_subdir}\"
         )
       endif()
     endforeach()

     if(NOT EXISTS \"\${_installed_bindir}/platforms/libqxcb.so\")
       message(FATAL_ERROR \"Bundled Linux Qt platform plugin is missing: \${_installed_bindir}/platforms/libqxcb.so\")
     endif()

     file(WRITE \"\${_installed_bindir}/qt.conf\" \"[Paths]\\nPrefix=..\\nPlugins=${CMAKE_INSTALL_BINDIR}\\nLibraries=${CMAKE_INSTALL_LIBDIR}\\n\")
     file(WRITE \"\${_installed_bindir}/qt-runtime-version.txt\" \"\${_qt_version}\\n\")

     message(STATUS \"Bundling Coin/SoQt runtime libs (Linux)\")

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
    ")

  # Qt plugins copied from the SDK may retain an SDK RUNPATH. For deployed
  # direct ELF executables, make their Qt dependencies resolve to <prefix>/lib.
  find_program(TONATIUHPP_PATCHELF_EXECUTABLE NAMES patchelf)
  if(NOT TONATIUHPP_PATCHELF_EXECUTABLE)
    message(STATUS "patchelf not found: required only for installing bundled Linux Qt plugins")
  endif()
  install(CODE
    "if(NOT EXISTS \"${TONATIUHPP_PATCHELF_EXECUTABLE}\")
       message(FATAL_ERROR \"Bundled Linux runtime install requires patchelf.\")
     endif()
     set(_installed_bindir \"\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}\")
     set(_installed_libdir \"\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}\")
     file(GLOB_RECURSE _installed_plugins \"\${_installed_bindir}/*.so\")
     foreach(_plugin IN LISTS _installed_plugins)
       get_filename_component(_plugin_dir \"\${_plugin}\" DIRECTORY)
       file(RELATIVE_PATH _rel_lib \"\${_plugin_dir}\" \"\${_installed_libdir}\")
       file(RELATIVE_PATH _rel_bin \"\${_plugin_dir}\" \"\${_installed_bindir}\")
       execute_process(
         COMMAND \"${TONATIUHPP_PATCHELF_EXECUTABLE}\" --set-rpath
                 \"$ORIGIN/\${_rel_lib}:$ORIGIN/\${_rel_bin}:$ORIGIN\" \"\${_plugin}\"
         RESULT_VARIABLE _patch_exit ERROR_VARIABLE _patch_error)
       if(NOT _patch_exit EQUAL 0)
         message(FATAL_ERROR \"Failed to set bundled plugin RUNPATH for \${_plugin}: \${_patch_error}\")
       endif()
     endforeach()

     # ELF RUNPATH is not inherited across indirect DT_NEEDED dependencies.
     # Bundled Qt/SoQt/Coin libraries must independently locate other
     # bundled .so files (e.g. libSoQt -> libQt6OpenGL).
     file(GLOB _bundled_libs \"\${_installed_libdir}/*.so*\")
     foreach(_lib IN LISTS _bundled_libs)
       if(IS_SYMLINK \"\${_lib}\")
         continue()
       endif()
       execute_process(
         COMMAND \"${TONATIUHPP_PATCHELF_EXECUTABLE}\" --set-rpath
                 \"$ORIGIN\" \"\${_lib}\"
         RESULT_VARIABLE _lib_patch_exit ERROR_VARIABLE _lib_patch_error)
       if(NOT _lib_patch_exit EQUAL 0)
         message(FATAL_ERROR
           \"Failed to set bundled library RUNPATH for \${_lib}: \${_lib_patch_error}\")
       endif()
     endforeach()"
  )
endif()

# ----------------------------
# 🚚 Windows runtime deployment
# ----------------------------
if(WIN32)
  # Put our third-party DLLs next to the exe on install (if you build them)
  set(_windows_thirdparty_bin "${CMAKE_CURRENT_LIST_DIR}/../third_party/_install/bin")
  install(CODE
    "set(_thirdparty_bin \"${_windows_thirdparty_bin}\")
     if(EXISTS \"\${_thirdparty_bin}\")
       file(GLOB _thirdparty_dlls \"\${_thirdparty_bin}/*.dll\")
       if(_thirdparty_dlls)
         file(INSTALL
           DESTINATION \"\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}\"
           TYPE FILE
           FILES \${_thirdparty_dlls}
         )
       else()
         message(STATUS \"No third-party DLLs found in \${_thirdparty_bin}; skipping install copy.\")
       endif()
     else()
       message(STATUS \"Third-party DLL directory not found; skipping install copy: \${_thirdparty_bin}\")
     endif()")

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
    # Deploy both permanent executables into the same installed bin root.
    # Generator expressions avoid Debug/Release filename guesses.
    install(CODE
      "set(_installed_bindir \"\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}\")
       foreach(_exe_name IN ITEMS
           \"$<TARGET_FILE_NAME:tonatiuhpp-gui>\"
           \"$<TARGET_FILE_NAME:tonatiuhpp-cli>\")
         set(_exe \"\${_installed_bindir}/\${_exe_name}\")
         if(NOT EXISTS \"\${_exe}\")
           message(FATAL_ERROR \"Missing installed Tonatiuh++ executable: \${_exe}\")
         endif()
         execute_process(
           COMMAND \"${WINDEPLOYQT_EXECUTABLE}\" \"\${_exe}\"
                   --no-translations --compiler-runtime --verbose 1
           RESULT_VARIABLE _wdq_rv OUTPUT_VARIABLE _wdq_out ERROR_VARIABLE _wdq_err)
         if(NOT _wdq_rv EQUAL 0)
           message(FATAL_ERROR \"windeployqt failed for \${_exe}: \${_wdq_err}\\n\${_wdq_out}\")
         endif()
       endforeach()"
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

# P2: Linux installs two direct ELF executables without compatibility wrappers.

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
