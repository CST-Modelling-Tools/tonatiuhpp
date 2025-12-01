if(NOT DEFINED CMAKE_POLICY_VERSION_MINIMUM)
  set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
endif()

cmake_minimum_required(VERSION 3.20)
project(soqt_smoketest LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets OpenGL OpenGLWidgets)
find_package(Coin REQUIRED)
find_package(SoQt REQUIRED)

add_executable(soqt_smoketest main.cpp)

# ----------------------------------------------------------
# macOS: Add include directories from Qt6 frameworks
# ----------------------------------------------------------
foreach(component Widgets OpenGL OpenGLWidgets Core Gui)
    get_target_property(_incs Qt6::${component} INTERFACE_INCLUDE_DIRECTORIES)
    if(_incs)
        include_directories(${_incs})
    endif()
endforeach()

# ----------------------------------------------------------
# macOS: Fix framework linking by adding -F paths
# ----------------------------------------------------------
if(APPLE)
    foreach(component Widgets OpenGL OpenGLWidgets Core Gui)
        get_target_property(_loc Qt6::${component} LOCATION)
        get_filename_component(_fw_dir "${_loc}/../.." REALPATH)
        message(STATUS "Qt framework dir for ${component}: ${_fw_dir}")
        target_link_directories(soqt_smoketest PRIVATE "${_fw_dir}")
        target_link_options(soqt_smoketest PRIVATE "-F${_fw_dir}")
    endforeach()
endif()

target_link_libraries(soqt_smoketest
  PRIVATE
    Qt6::Widgets
    Qt6::OpenGL
    Qt6::OpenGLWidgets
    Coin::Coin
    SoQt::SoQt
)

if(MSVC)
  target_compile_options(soqt_smoketest PRIVATE /permissive- /Zc:__cplusplus)
  target_compile_definitions(soqt_smoketest PRIVATE SOQT_DLL)
endif()