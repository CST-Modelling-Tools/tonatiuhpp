# TONATIUHPP_P1_CENTRAL_WIN_RUNTIME - generated per-config invocation.
# Variables supplied by source/CMakeLists.txt file(GENERATE):
#   TONATIUHPP_P1_RUNTIME_DIR, TONATIUHPP_P1_DLLS,
#   TONATIUHPP_P1_QT_CORE_DLL, TONATIUHPP_P1_THIRDPARTY_BIN.
# This is a build-tree-only operation; installation rules are not changed.

foreach(_required IN ITEMS TONATIUHPP_P1_RUNTIME_DIR TONATIUHPP_P1_QT_CORE_DLL)
  if(NOT DEFINED ${_required} OR "${${_required}}" STREQUAL "")
    message(FATAL_ERROR "Windows runtime stage requires ${_required}")
  endif()
endforeach()
if(NOT EXISTS "${TONATIUHPP_P1_QT_CORE_DLL}")
  message(FATAL_ERROR "The configured Qt Core DLL does not exist: ${TONATIUHPP_P1_QT_CORE_DLL}")
endif()

file(MAKE_DIRECTORY "${TONATIUHPP_P1_RUNTIME_DIR}")
get_filename_component(_qt_bin "${TONATIUHPP_P1_QT_CORE_DLL}" DIRECTORY)
get_filename_component(_qt_prefix "${_qt_bin}" DIRECTORY)
set(_qt_plugin_source "${_qt_prefix}/plugins")
if(NOT EXISTS "${_qt_plugin_source}/platforms")
  message(FATAL_ERROR "Qt platform plugins missing under ${_qt_plugin_source}/platforms")
endif()

# Stage external DLLs in one place, without copying any Tonatiuh++ DLLs.
# Target runtime paths include our own DLLs, which may not exist yet because
# this stage is a PREREQUISITE of the executables, not a post-link action.
set(_external_dlls "${TONATIUHPP_P1_QT_CORE_DLL}")
if(DEFINED TONATIUHPP_P1_DLLS AND NOT "${TONATIUHPP_P1_DLLS}" STREQUAL "")
  list(APPEND _external_dlls ${TONATIUHPP_P1_DLLS})
endif()
if(IS_DIRECTORY "${TONATIUHPP_P1_THIRDPARTY_BIN}")
  file(GLOB _thirdparty_dlls LIST_DIRECTORIES FALSE "${TONATIUHPP_P1_THIRDPARTY_BIN}/*.dll")
  list(APPEND _external_dlls ${_thirdparty_dlls})
endif()
# Some Qt plugin and Coin/SoQt runtime imports are not accurately reflected in
# older third-party CMake package metadata. Include only small, known Qt helper
# modules (if installed), never the entire Qt bin directory (WebEngine etc.).
foreach(_qt_helper IN ITEMS QmlModels QmlWorkerScript OpenGL OpenGLWidgets
                            Svg SvgWidgets)
  file(GLOB _helper_dlls LIST_DIRECTORIES FALSE
       "${_qt_bin}/Qt6${_qt_helper}.dll"
       "${_qt_bin}/Qt6${_qt_helper}d.dll")
  list(APPEND _external_dlls ${_helper_dlls})
endforeach()
list(REMOVE_DUPLICATES _external_dlls)

file(TO_CMAKE_PATH "${TONATIUHPP_P1_RUNTIME_DIR}" _output_normalized)
string(TOLOWER "${_output_normalized}" _output_normalized_lower)
set(_seen_names)
set(_seen_sources)
set(_copied 0)
foreach(_source IN LISTS _external_dlls)
  if("${_source}" STREQUAL "")
    continue()
  endif()
  file(TO_CMAKE_PATH "${_source}" _source_normalized)
  string(TOLOWER "${_source_normalized}" _source_lower)
  # A generated internal target is already emitted directly to the runtime.
  string(FIND "${_source_lower}" "${_output_normalized_lower}/" _is_internal)
  if(_is_internal EQUAL 0)
    continue()
  endif()
  if(NOT EXISTS "${_source}")
    message(FATAL_ERROR "Configured external runtime dependency missing: ${_source}")
  endif()
  get_filename_component(_filename "${_source}" NAME)
  string(TOLOWER "${_filename}" _filename_lower)
  list(FIND _seen_names "${_filename_lower}" _duplicate_index)
  if(NOT _duplicate_index EQUAL -1)
    list(GET _seen_sources ${_duplicate_index} _first_source)
    file(SHA256 "${_first_source}" _first_hash)
    file(SHA256 "${_source}" _second_hash)
    if(NOT _first_hash STREQUAL _second_hash)
      message(FATAL_ERROR "Conflicting runtime DLL basename ${_filename}: ${_first_source} and ${_source}")
    endif()
    continue()
  endif()
  list(APPEND _seen_names "${_filename_lower}")
  list(APPEND _seen_sources "${_source}")
  file(COPY_FILE "${_source}" "${TONATIUHPP_P1_RUNTIME_DIR}/${_filename}" ONLY_IF_DIFFERENT)
  math(EXPR _copied "${_copied}+1")
endforeach()

# Keep Qt's own dynamic plugin tree separate from the Tonatiuh++ scientific
# plugin tree, so QPluginLoader does not scan qwindows.dll as a physics plugin.
foreach(_category IN ITEMS platforms imageformats iconengines styles tls
                            sqldrivers networkinformation platformthemes generic)
  if(NOT IS_DIRECTORY "${_qt_plugin_source}/${_category}")
    continue()
  endif()
  file(GLOB _qt_plugins LIST_DIRECTORIES FALSE "${_qt_plugin_source}/${_category}/*.dll")
  if(_qt_plugins)
    file(MAKE_DIRECTORY "${TONATIUHPP_P1_RUNTIME_DIR}/qt-plugins/${_category}")
    foreach(_plugin IN LISTS _qt_plugins)
      get_filename_component(_name "${_plugin}" NAME)
      file(COPY_FILE "${_plugin}"
           "${TONATIUHPP_P1_RUNTIME_DIR}/qt-plugins/${_category}/${_name}"
           ONLY_IF_DIFFERENT)
    endforeach()
  endif()
endforeach()

# Only the BUILD tree receives this file. External installations continue
# using their own packaging/Qt deployment rules until P3.
file(WRITE "${TONATIUHPP_P1_RUNTIME_DIR}/qt.conf"
  "[Paths]\nPrefix = .\nPlugins = qt-plugins\n")

message(STATUS "P1 Windows runtime staged: ${_copied} external DLL files into ${TONATIUHPP_P1_RUNTIME_DIR}")
