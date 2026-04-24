if(NOT DEFINED CMAKE_POLICY_VERSION_MINIMUM)
  set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
endif()

cmake_minimum_required(VERSION 3.20)
project(updater_reader_test LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 REQUIRED COMPONENTS Core)

get_filename_component(REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)

add_executable(updater_reader_test
  main.cpp
  "${REPO_ROOT}/source/application/updater/UpdateReader.cpp"
)

target_include_directories(updater_reader_test PRIVATE
  "${REPO_ROOT}/source/application/updater"
)

target_link_libraries(updater_reader_test PRIVATE Qt6::Core)

include(CTest)
if(BUILD_TESTING)
  if(MSVC)
    add_test(
      NAME updater_reader_test
      COMMAND "${CMAKE_COMMAND}" -E env "PATH=$<TARGET_FILE_DIR:Qt6::Core>;$ENV{PATH}" "$<TARGET_FILE:updater_reader_test>"
    )
  else()
    add_test(NAME updater_reader_test COMMAND updater_reader_test)
  endif()
endif()
