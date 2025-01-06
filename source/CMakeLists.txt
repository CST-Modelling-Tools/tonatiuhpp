# ----------------------------
# 🔧 General Build Configuration
# ----------------------------
cmake_minimum_required(VERSION 3.28)
project(TonatiuhXX VERSION 0.1.8.16)

# Build Configuration
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Default Build Type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Qt AutoTools
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# ----------------------------
# 📚 Dependencies
# ----------------------------
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets PrintSupport Qml Concurrent)
find_package(Eigen3 REQUIRED NO_MODULE)
find_library(COIN3D_LIBRARY Coin REQUIRED)
find_library(SOQT_LIBRARY SoQt REQUIRED)
find_path(COIN3D_INCLUDE_DIR Inventor REQUIRED)
find_path(SOQT_INCLUDE_DIR Inventor/Qt REQUIRED)

# ----------------------------
# 🛠️ Global Definitions
# ----------------------------
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
# ✅ Pass Global Dependencies to Subdirectories
# ----------------------------
set(GLOBAL_INCLUDE_DIRS
    ${COIN3D_INCLUDE_DIR}
    ${SOQT_INCLUDE_DIR}
    ${EIGEN3_INCLUDE_DIRS}
)

set(GLOBAL_LIBRARIES
    ${COIN3D_LIBRARY}
    ${SOQT_LIBRARY}
    Eigen3::Eigen
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::PrintSupport
    Qt6::Qml
)

# Pass dependencies as global properties
set_property(GLOBAL PROPERTY GLOBAL_INCLUDE_DIRS ${GLOBAL_INCLUDE_DIRS})
set_property(GLOBAL PROPERTY GLOBAL_LIBRARIES ${GLOBAL_LIBRARIES})

# ----------------------------
# ✅ Final Status Message
# ----------------------------
message(STATUS "CMake configuration completed successfully!")