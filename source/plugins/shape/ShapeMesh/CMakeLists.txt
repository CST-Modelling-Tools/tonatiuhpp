cmake_minimum_required(VERSION 3.28)

set(ProjectName ShapeMesh)
project(${ProjectName} VERSION 0.1.8.16)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Assuming the necessary Qt and Coin3D libraries are found in the top-level CMakeLists.txt

# Include directories, assuming relative paths will be inherited from top-level
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Header files
set(HEADERS
    BVH.h
    ShapeMesh.h
    Triangle.h
)

# Source files
set(SOURCES 
    BVH.cpp
    ShapeMesh.cpp
    Triangle.cpp
)

# Resource files, if any
set(RESOURCES resources.qrc)

# Add the plugin as a library
add_library(${PROJECT_NAME} SHARED ${HEADERS} ${SOURCES} ${RESOURCES})

# Link libraries using globally defined variables and target links
target_link_libraries(${PROJECT_NAME} PRIVATE 
    Coin::Coin
    SoQt::SoQt
    Qt6::Core 
    Qt6::Gui 
    Qt6::Widgets 
    Qt6::Qml
    TonatiuhLibraries
    TonatiuhKernel
)

# Find the required Qt components
find_package(Qt6 COMPONENTS Core Gui Widgets Qml REQUIRED)

# Add install rules for all targets
install(TARGETS ${ProjectName}
    RUNTIME DESTINATION "${GLOBAL_INSTALL_BIN_DIR}"
    LIBRARY DESTINATION "${GLOBAL_INSTALL_BIN_DIR}"
    ARCHIVE DESTINATION "${GLOBAL_INSTALL_BIN_DIR}"
)