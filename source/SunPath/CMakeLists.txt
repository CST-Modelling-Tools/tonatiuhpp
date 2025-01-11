cmake_minimum_required(VERSION 3.28)

# Project configuration
set(ProjectName SunPath)
project(${ProjectName})

# Set the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Header files
set(HEADERS 
    calculators/MB/sunpos.h
    calculators/NREL/spa.h
    calculators/RG/Sun_position_algorithms.h
    calculators/Location.h
    calculators/SunCalculator.h
    calculators/SunCalculatorET.h
    calculators/SunCalculatorMB.h
    calculators/SunCalculatorNREL.h
    calculators/SunCalculatorRG.h
    data/FormatTMY.h
    data/FormatWSN.h
    data/SkyModel.h
    data/SkyModelPI.h
    data/SunFunctor.h
    math/geometry/Interval.h
    math/geometry/vec2d.h
    math/geometry/vec3d.h
    math/matrices/Matrix.h
    math/matrices/MatrixNR.h
    math/sampling/ErrorAnalysis.h
    math/sampling/Interpolator.h
    math/sampling/Summator.h
    math/math.h
    samplers/SkyKernels.h
    samplers/SkySampler.h
    samplers/SunSpatial.h
    samplers/SunTemporal.h
    samplers/TimeSampler.h
    SunPath.h
)

# Source files
set(SOURCES 
    calculators/MB/SunPos.cpp
    calculators/MB/SunPos2.cpp
    calculators/NREL/spa.cpp
    calculators/Location.cpp
    calculators/SunCalculator.cpp
    calculators/SunCalculatorET.cpp
    calculators/SunCalculatorMB.cpp
    calculators/SunCalculatorNREL.cpp
    calculators/SunCalculatorRG.cpp
    data/FormatTMY.cpp
    data/FormatWSN.cpp
    data/SkyModel.cpp
    data/SkyModelPI.cpp
    data/SunFunctor.cpp
    math/geometry/Interval.cpp
    math/geometry/vec2d.cpp
    math/geometry/vec3d.cpp
    math/matrices/MatrixNR.cpp
    math/sampling/ErrorAnalysis.cpp
    math/sampling/Interpolator.cpp
    math/sampling/Summator.cpp
    math/math.cpp
    samplers/SkyKernels.cpp
    samplers/SkySampler.cpp
    samplers/SunSpatial.cpp
    samplers/SunTemporal.cpp
    samplers/TimeSampler.cpp
)

# Create the library
add_library(${ProjectName} SHARED ${SOURCES} ${HEADERS})

# Target-specific include directories
target_include_directories(${ProjectName}
    PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR} 
         ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(${ProjectName} 
    PRIVATE 
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::PrintSupport
    Qt6::Qml
    Eigen3::Eigen
    ${COIN3D_LIBRARY}
    ${SOQT_LIBRARY}
)

# Add compile definitions if necessary
target_compile_definitions(${ProjectName} PRIVATE SunPath_EXPORT)

install(TARGETS ${ProjectName}
    LIBRARY DESTINATION "${CMAKE_INSTALL_PREFIX}/bin"
    RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}/bin"
)

# Add install rules for all targets
install(TARGETS ${ProjectName}
    RUNTIME DESTINATION "${GLOBAL_INSTALL_BIN_DIR}"
    LIBRARY DESTINATION "${GLOBAL_INSTALL_BIN_DIR}"
    ARCHIVE DESTINATION "${GLOBAL_INSTALL_BIN_DIR}"
)