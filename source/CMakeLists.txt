# ----------------------------
# 🔧 General Build Configuration
# ----------------------------
cmake_minimum_required(VERSION 3.28)
cmake_policy(SET CMP0074 NEW)

project(TonatiuhXX VERSION 0.1.8.16)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# ----------------------------
# 📚 Dependencies
# ----------------------------

# Find Qt6 modules (installed system-wide or via CMAKE_PREFIX_PATH)
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets PrintSupport Qml Concurrent)

# -------- Eigen3 ----------
if(DEFINED ENV{EIGEN3_ROOT})
    set(Eigen3_DIR "$ENV{EIGEN3_ROOT}/share/eigen3/cmake")
endif()
find_package(Eigen3 REQUIRED)

if(Eigen3_FOUND AND TARGET Eigen3::Eigen)
    get_target_property(EIGEN3_INCLUDE_DIR Eigen3::Eigen INTERFACE_INCLUDE_DIRECTORIES)
    list(APPEND GLOBAL_LIBRARIES Eigen3::Eigen)
elseif(NOT EIGEN3_INCLUDE_DIR)
    message(FATAL_ERROR "Eigen3 headers not found. Set EIGEN3_ROOT or verify installation.")
endif()

# -------- Coin3D ----------
if(DEFINED ENV{COIN3D_ROOT})
    set(COIN3D_DIR "$ENV{COIN3D_ROOT}")
endif()

find_library(COIN3D_LIBRARY NAMES Coin4 Coin libCoin
    PATHS $ENV{COIN3D_ROOT}/lib /usr/lib /usr/local/lib
)
find_path(COIN3D_INCLUDE_DIR Inventor
    PATHS $ENV{COIN3D_ROOT}/include /usr/include /usr/local/include
)

if(NOT COIN3D_LIBRARY OR NOT COIN3D_INCLUDE_DIR)
    message(FATAL_ERROR "Coin3D: Missing library or headers.")
endif()

# -------- SoQt ----------
if(DEFINED ENV{SOQT_ROOT})
    set(SOQT_DIR "$ENV{SOQT_ROOT}")
endif()

find_library(SOQT_LIBRARY NAMES SoQt1 SoQt libSoQt
    PATHS $ENV{SOQT_ROOT}/lib /usr/lib /usr/local/lib
)
find_path(SOQT_INCLUDE_DIR Inventor/Qt
    PATHS $ENV{SOQT_ROOT}/include /usr/include /usr/local/include
)

if(NOT SOQT_LIBRARY OR NOT SOQT_INCLUDE_DIR)
    message(FATAL_ERROR "SoQt: Missing library or headers.")
endif()

# ----------------------------
# 🛠️ Compiler Definitions
# ----------------------------
if(WIN32)
    add_compile_definitions(SOQT_DLL)
elseif(UNIX)
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
# 📦 Installation
# ----------------------------

# Define default install prefix with platform awareness
if(DEFINED ENV{TonatiuhXXInstallDir})
    set(CMAKE_INSTALL_PREFIX $ENV{TonatiuhXXInstallDir} CACHE PATH "Installation directory" FORCE)
elseif(WIN32)
    set(CMAKE_INSTALL_PREFIX "C:/TonatiuhXX" CACHE PATH "Installation directory" FORCE)
else()
    set(CMAKE_INSTALL_PREFIX "/opt/TonatiuhXX" CACHE PATH "Installation directory" FORCE)
endif()

message(STATUS "Installation directory: ${CMAKE_INSTALL_PREFIX}")

# Install target layout
set(GLOBAL_INSTALL_BIN_DIR "${CMAKE_INSTALL_PREFIX}/bin")
set(GLOBAL_INSTALL_INCLUDE_DIR "${CMAKE_INSTALL_PREFIX}/include")
set(GLOBAL_INSTALL_EXAMPLES_DIR "${CMAKE_INSTALL_PREFIX}/examples")
set(GLOBAL_INSTALL_HELP_DIR "${CMAKE_INSTALL_PREFIX}/help")
set(GLOBAL_INSTALL_RESOURCES_DIR "${CMAKE_INSTALL_PREFIX}/resources")

# Install main binary
install(TARGETS ${ProjectName}
    RUNTIME DESTINATION "${GLOBAL_INSTALL_BIN_DIR}"
    LIBRARY DESTINATION "${GLOBAL_INSTALL_BIN_DIR}"
    ARCHIVE DESTINATION "${GLOBAL_INSTALL_BIN_DIR}"
)

# Install project resources (optional)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/../examples/
    DESTINATION "${GLOBAL_INSTALL_EXAMPLES_DIR}"
    FILES_MATCHING PATTERN "*.*"
)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/../help/
    DESTINATION "${GLOBAL_INSTALL_HELP_DIR}"
    FILES_MATCHING PATTERN "*.*"
)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/../resources/
    DESTINATION "${GLOBAL_INSTALL_RESOURCES_DIR}"
    FILES_MATCHING PATTERN "*.*"
)

# ----------------------------
# ✅ Final Status
# ----------------------------
message(STATUS "CMake configuration completed successfully!")
message(STATUS "Dependencies:")
message(STATUS "  Eigen3: ${EIGEN3_INCLUDE_DIR}")
message(STATUS "  Coin3D: ${COIN3D_INCLUDE_DIR}")
message(STATUS "  SoQt: ${SOQT_INCLUDE_DIR}")