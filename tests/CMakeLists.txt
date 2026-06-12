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
elseif(WIN32)
  message(WARNING
    "TONATIUHPP_TEST_EXECUTABLE is not set; skipping headless CTest smoke tests "
    "on Windows. Configure with -DTONATIUHPP_TEST_EXECUTABLE=<installed tonatiuhpp.exe> "
    "to test an installed runtime."
  )
  set(TONATIUHPP_ENABLE_HEADLESS_SMOKE_TESTS OFF)
else()
  if(NOT TARGET tonatiuhpp)
    message(FATAL_ERROR "The tonatiuhpp target must exist before enabling build-tree tests.")
  endif()

  set(TONATIUHPP_TEST_EXE "$<TARGET_FILE:tonatiuhpp>")
  set(TONATIUHPP_TEST_WORKING_DIR "$<TARGET_FILE_DIR:tonatiuhpp>")
  message(STATUS "Headless CTest smoke tests will use the build-tree tonatiuhpp executable.")
endif()

function(tonatiuhpp_set_headless_test_properties test_name)
  set_tests_properties("${test_name}" PROPERTIES
    LABELS "headless;smoke"
    WORKING_DIRECTORY "${TONATIUHPP_TEST_WORKING_DIR}"
  )
endfunction()

if(TONATIUHPP_ENABLE_HEADLESS_SMOKE_TESTS)
  add_test(
    NAME headless.help
    COMMAND "${TONATIUHPP_TEST_EXE}" --headless --help
  )
  tonatiuhpp_set_headless_test_properties(headless.help)
  set_tests_properties(headless.help PROPERTIES
    PASS_REGULAR_EXPRESSION "Tonatiuh\\+\\+ headless mode"
  )

  add_test(
    NAME headless.invalid_arguments
    COMMAND "${CMAKE_COMMAND}"
      "-DTEST_EXECUTABLE=${TONATIUHPP_TEST_EXE}"
      "-DTEST_ARGUMENTS=--headless|not-a-command"
      "-DEXPECTED_EXIT_CODE=2"
      "-DTEST_WORKING_DIRECTORY=${TONATIUHPP_TEST_WORKING_DIR}"
      -P "${CMAKE_CURRENT_LIST_DIR}/cmake/expect_exit_code.cmake"
  )
  tonatiuhpp_set_headless_test_properties(headless.invalid_arguments)

  set(_tonatiuhpp_cylinder_scene "${CMAKE_CURRENT_LIST_DIR}/../examples/benchmarks/cylinder.tnhpp")
  if(EXISTS "${_tonatiuhpp_cylinder_scene}")
    add_test(
      NAME headless.validate_cylinder_scene
      COMMAND "${TONATIUHPP_TEST_EXE}" --headless validate-scene "${_tonatiuhpp_cylinder_scene}"
    )
    tonatiuhpp_set_headless_test_properties(headless.validate_cylinder_scene)
    set_tests_properties(headless.validate_cylinder_scene PROPERTIES
      PASS_REGULAR_EXPRESSION "Scene validation succeeded"
    )
  endif()
endif()
