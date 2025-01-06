cmake_minimum_required(VERSION 3.28)
project(Tonatiuh++_Plugins)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../libraries)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../kernel)
include_directories(${COIN3D_INCLUDE_DIR})
include_directories(${SOQT_INCLUDE_DIR})

# Add each plugin subdirectory
add_subdirectory(air)
# add_subdirectory(component) # Uncomment this if you want to include the 'component' plugin
add_subdirectory(material)
add_subdirectory(photons)
add_subdirectory(random)
add_subdirectory(shape)
add_subdirectory(sun)
# add_subdirectory(trackers)