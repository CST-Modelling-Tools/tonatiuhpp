cmake_minimum_required(VERSION 3.20)
project(soqt_smoketest LANGUAGES CXX)

# Allow packages that request old policy versions (Coin does).
# Works with CMake >= 3.27 and especially 4.x.
if(NOT DEFINED CMAKE_POLICY_VERSION_MINIMUM)
  set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets OpenGL OpenGLWidgets)
find_package(Coin REQUIRED)
find_package(SoQt REQUIRED)

add_executable(soqt_smoketest main.cpp)
target_link_libraries(soqt_smoketest
  PRIVATE Qt6::Widgets Qt6::OpenGL Qt6::OpenGLWidgets Coin::Coin SoQt::SoQt)

if(MSVC)
  target_compile_options(soqt_smoketest PRIVATE /permissive- /Zc:__cplusplus)
  target_compile_definitions(soqt_smoketest PRIVATE SOQT_DLL)
endif()