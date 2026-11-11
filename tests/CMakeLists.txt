include(FetchContent)

find_package(GTest CONFIG QUIET)

if(NOT GTest_FOUND)
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
    GIT_SHALLOW TRUE
  )
  FetchContent_MakeAvailable(googletest)
endif()

include(GoogleTest)

add_subdirectory(unit/libraries/math)
add_subdirectory(unit/kernel/random)
add_subdirectory(scientific)

set(TONATIUHPP_ENABLE_HEADLESS_SMOKE_TESTS ON)

if(NOT "${TONATIUHPP_TEST_EXECUTABLE}" STREQUAL "")
  if(IS_ABSOLUTE "${TONATIUHPP_TEST_EXECUTABLE}")
    set(TONATIUHPP_TEST_EXE "${TONATIUHPP_TEST_EXECUTABLE}")
  else()
    get_filename_component(TONATIUHPP_TEST_EXE
      "${TONATIUHPP_TEST_EXECUTABLE}"
      ABSOLUTE
      BASE_DIR "${CMAKE_BINARY_DIR}"
    )
  endif()
  get_filename_component(TONATIUHPP_TEST_WORKING_DIR "${TONATIUHPP_TEST_EXE}" DIRECTORY)
  message(STATUS "Headless CTest smoke tests will use TONATIUHPP_TEST_EXECUTABLE: ${TONATIUHPP_TEST_EXE}")
else()
  if(NOT TARGET tonatiuhpp-cli)
    message(FATAL_ERROR "The tonatiuhpp-cli target must exist before enabling build-tree tests.")
  endif()

  # Generator expressions resolve the configured Release executable on
  # multi-config MSVC builds as well as Ninja/Linux/macOS builds.
  set(TONATIUHPP_TEST_EXE "$<TARGET_FILE:tonatiuhpp-cli>")
  set(TONATIUHPP_TEST_WORKING_DIR "$<TARGET_FILE_DIR:tonatiuhpp-cli>")
  message(STATUS "Headless CTest smoke tests use the build-tree tonatiuhpp-cli target.")
endif()

function(tonatiuhpp_set_headless_test_properties test_name)
  set_tests_properties("${test_name}" PROPERTIES
    LABELS "headless;smoke"
    WORKING_DIRECTORY "${TONATIUHPP_TEST_WORKING_DIR}"
  )
endfunction()

# Included registrations share this directory's CMake scope, binary root and
# existing fixture paths. Included files use TONATIUHPP_TESTS_ROOT for
# source paths, not their own CMAKE_CURRENT_LIST_DIR.
set(TONATIUHPP_TESTS_ROOT "${CMAKE_CURRENT_LIST_DIR}")
if(TONATIUHPP_ENABLE_HEADLESS_SMOKE_TESTS)
  include("${TONATIUHPP_TESTS_ROOT}/integration/cli/CTest.cmake")
  include("${TONATIUHPP_TESTS_ROOT}/integration/scientific/CTest.cmake")
endif()

include("${TONATIUHPP_TESTS_ROOT}/integration/cli/ProcessRepeatability.cmake")
include("${TONATIUHPP_TESTS_ROOT}/architecture/CTest.cmake")
