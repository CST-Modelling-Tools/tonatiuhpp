cmake_minimum_required(VERSION 3.28)

# ----------------------------
# 🎛 Project configuration
# ----------------------------
# This directory builds all Tonatiuh++ plugin libraries.
set(ProjectName TonatiuhPP_Plugins)
project(${ProjectName})

# C++ standard (redundant with top-level, but harmless)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# We rely on GNUInstallDirs for CMAKE_INSTALL_BINDIR
include(GNUInstallDirs)

# ----------------------------
# 📚 Include directories
# ----------------------------
# NOTE: these are legacy, directory-wide include paths.
# New code should prefer target_include_directories in each subdir.
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../libraries)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../kernel)
include_directories(${COIN3D_INCLUDE_DIR})
include_directories(${SOQT_INCLUDE_DIR})

# ----------------------------
# 🔌 Add each plugin subdirectory
# ----------------------------
add_subdirectory(air)
# add_subdirectory(component)
add_subdirectory(material)
add_subdirectory(photons)
add_subdirectory(shape)
add_subdirectory(sun)
# add_subdirectory(trackers)

# ----------------------------
# 🚚 Install plugin libraries next to the main executable
# ----------------------------
# These names come from the libraries observed in <prefix>/lib:
#   libAirMirval.so              -> target AirMirval
#   libMaterialSpecular.so       -> target MaterialSpecular
#   libPhotonsFile.so            -> target PhotonsFile
#   libShapeElliptic.so          -> target ShapeElliptic
#   libShapeFunctionXYZ.so       -> target ShapeFunctionXYZ
#   libShapeFunctionZ.so         -> target ShapeFunctionZ
#   libShapeHyperbolic.so        -> target ShapeHyperbolic
#   libShapeMapN.so              -> target ShapeMapN
#   libShapeMesh.so              -> target ShapeMesh
#   libSunBuie.so                -> target SunBuie
#   libSunGaussian.so            -> target SunGaussian
#
# If you add new plugins later, just append their CMake target names
# to this list.

set(TONATIUHPP_PLUGIN_TARGETS
    AirMirval
    MaterialSpecular
    PhotonsFile
    ShapeElliptic
    ShapeFunctionXYZ
    ShapeFunctionZ
    ShapeHyperbolic
    ShapeMapN
    ShapeMesh
    SunBuie
    SunGaussian
)

foreach(_tgt IN LISTS TONATIUHPP_PLUGIN_TARGETS)
    if(TARGET ${_tgt})
        message(STATUS "Installing Tonatiuh++ plugin target: ${_tgt}")
        install(
            TARGETS ${_tgt}
            RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"  # .dll on Windows
            LIBRARY DESTINATION "${CMAKE_INSTALL_BINDIR}"  # .so/.dylib on Linux/macOS
            ARCHIVE DESTINATION "${CMAKE_INSTALL_BINDIR}"  # harmless
        )
    else()
        message(STATUS "Tonatiuh++ plugin target not found (skipping): ${_tgt}")
    endif()
endforeach()

# TONATIUHPP_P1_CENTRAL_WIN_RUNTIME: one build-tree scientific plugin tree.
# Keep Linux/macOS output and external install rules unchanged in P1.
if(WIN32)
    foreach(_tonatiuhpp_p1_group IN ITEMS air material photons shape sun)
        if(_tonatiuhpp_p1_group STREQUAL "air")
            set(_tonatiuhpp_p1_group_targets AirMirval)
        elseif(_tonatiuhpp_p1_group STREQUAL "material")
            set(_tonatiuhpp_p1_group_targets MaterialSpecular)
        elseif(_tonatiuhpp_p1_group STREQUAL "photons")
            set(_tonatiuhpp_p1_group_targets PhotonsFile)
        elseif(_tonatiuhpp_p1_group STREQUAL "shape")
            set(_tonatiuhpp_p1_group_targets ShapeElliptic ShapeFunctionXYZ
                ShapeFunctionZ ShapeHyperbolic ShapeMapN ShapeMesh)
        else()
            set(_tonatiuhpp_p1_group_targets SunBuie SunGaussian)
        endif()
        foreach(_tonatiuhpp_p1_plugin IN LISTS _tonatiuhpp_p1_group_targets)
            if(NOT TARGET ${_tonatiuhpp_p1_plugin})
                message(FATAL_ERROR "P1 missing plugin: ${_tonatiuhpp_p1_plugin}")
            endif()
            set(_tonatiuhpp_p1_dest
                "${CMAKE_BINARY_DIR}/bin/$<CONFIG>/plugins/${_tonatiuhpp_p1_group}")
            set_target_properties(${_tonatiuhpp_p1_plugin} PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY "${_tonatiuhpp_p1_dest}"
                LIBRARY_OUTPUT_DIRECTORY "${_tonatiuhpp_p1_dest}")
        endforeach()
    endforeach()
endif()
