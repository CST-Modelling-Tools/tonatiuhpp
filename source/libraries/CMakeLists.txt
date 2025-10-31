cmake_minimum_required(VERSION 3.28)

# Project name and version
set(ProjectName TonatiuhLibraries)
project(${ProjectName} VERSION 1.0.0)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Header files
set(HEADERS 
    TonatiuhLibraries.h 
    auxiliary/tiny_obj_loader.h 
    auxiliary/Trace.h 
    Coin3D/ContainerEditorMFVec2.h 
    Coin3D/ContainerViewerMFVec2.h 
    Coin3D/DoubleValuesDelegate.h 
    Coin3D/FieldEditor.h 
    Coin3D/MFVec2.h 
    Coin3D/UserMField.h 
    Coin3D/UserSField.h 
    DistMesh/PolygonMesh.h 
    DistMesh/constants.h 
    DistMesh/delaunator.h 
    DistMesh/distance_function.h 
    DistMesh/distmesh.h 
    DistMesh/functional.h 
    DistMesh/helper.h 
    DistMesh/triangulation.h 
    DistMesh/utils.h 
    math/1D/Grid.h 
    math/1D/Interval.h 
    math/1D/IntervalPeriodic.h 
    math/2D/Box2D.h 
    math/2D/Interpolation2D.h 
    math/2D/Matrix2D.h
    math/2D/vec2d.h
    math/2D/vec2i.h
    math/3D/Box3D.h
    math/3D/Matrix4x4.h
    math/3D/Ray.h
    math/3D/Transform.h
    math/3D/Transform3D.h
    math/3D/vec3d.h
    math/gcf.h
    QCustomPlot/qcustomplot.h
    sun/sunpos.h
)

# Source files
set(SOURCES 
    auxiliary/Trace.cpp
    auxiliary/tiny_obj_loader.cpp
    Coin3D/ContainerEditorMFVec2.cpp
    Coin3D/ContainerViewerMFVec2.cpp
    Coin3D/DoubleValuesDelegate.cpp
    Coin3D/MFVec2.cpp
    Coin3D/UserMField.cpp
    Coin3D/UserSField.cpp
    DistMesh/PolygonMesh.cpp
    DistMesh/distance_function.cpp
    DistMesh/distmesh.cpp
    DistMesh/functional.cpp
    DistMesh/triangulation.cpp
    DistMesh/utils.cpp
    math/1D/Grid.cpp
    math/1D/Interval.cpp
    math/1D/IntervalPeriodic.cpp
    math/2D/Box2D.cpp
    math/2D/vec2d.cpp
    math/2D/vec2i.cpp
    math/3D/Box3D.cpp
    math/3D/Matrix4x4.cpp
    math/3D/Transform.cpp
    math/3D/Transform3D.cpp
    math/3D/vec3d.cpp
    math/gcf.cpp
    QCustomPlot/qcustomplot.cpp
    sun/sunpos.cpp
)

# Define library target
add_library(${ProjectName} SHARED ${SOURCES} ${HEADERS})

# Include directories for the target
target_include_directories(${ProjectName} 
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/..
        ${COIN3D_INCLUDE_DIR}
)

# Link inherited libraries, including Eigen
target_link_libraries(${ProjectName}
    PRIVATE
        Coin::Coin
        Qt6::PrintSupport
        Eigen3::Eigen
)

# Add compile definitions if necessary
target_compile_definitions(${PROJECT_NAME} PRIVATE TONATIUH_LIBRARIES_EXPORT SunPath_EXPORT)

# Add install rules for all targets
install(TARGETS ${ProjectName}
    RUNTIME DESTINATION "${GLOBAL_INSTALL_BIN_DIR}"
    LIBRARY DESTINATION "${GLOBAL_INSTALL_BIN_DIR}"
    ARCHIVE DESTINATION "${GLOBAL_INSTALL_BIN_DIR}"
)