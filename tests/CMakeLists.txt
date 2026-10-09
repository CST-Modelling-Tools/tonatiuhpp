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
  if(NOT TARGET tonatiuhpp)
    message(FATAL_ERROR "The tonatiuhpp target must exist before enabling build-tree tests.")
  endif()

  # Generator expressions resolve the configured Release executable on
  # multi-config MSVC builds as well as Ninja/Linux/macOS builds.
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

  add_test(
    NAME headless.run_script_missing_path
    COMMAND "${CMAKE_COMMAND}"
      "-DTEST_EXECUTABLE=${TONATIUHPP_TEST_EXE}"
      "-DTEST_ARGUMENTS=--headless|run-script"
      "-DEXPECTED_EXIT_CODE=2"
      "-DTEST_WORKING_DIRECTORY=${TONATIUHPP_TEST_WORKING_DIR}"
      -P "${CMAKE_CURRENT_LIST_DIR}/cmake/expect_exit_code.cmake"
  )
  tonatiuhpp_set_headless_test_properties(headless.run_script_missing_path)

  add_test(
    NAME headless.run_script_missing_file
    COMMAND "${CMAKE_COMMAND}"
      "-DTEST_EXECUTABLE=${TONATIUHPP_TEST_EXE}"
      "-DTEST_ARGUMENTS=--headless|run-script|${CMAKE_CURRENT_BINARY_DIR}/missing_headless_script.tnhpps"
      "-DEXPECTED_EXIT_CODE=1"
      "-DTEST_WORKING_DIRECTORY=${TONATIUHPP_TEST_WORKING_DIR}"
      -P "${CMAKE_CURRENT_LIST_DIR}/cmake/expect_exit_code.cmake"
  )
  tonatiuhpp_set_headless_test_properties(headless.run_script_missing_file)

  add_test(
    NAME headless.run_script_rejects_gui_api
    COMMAND "${CMAKE_COMMAND}"
      "-DTEST_EXECUTABLE=${TONATIUHPP_TEST_EXE}"
      "-DTEST_WORKING_DIRECTORY=${TONATIUHPP_TEST_WORKING_DIR}"
      "-DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}/headless_run_script_errors"
      -P "${CMAKE_CURRENT_LIST_DIR}/cmake/run_headless_script_error_smoke.cmake"
  )
  tonatiuhpp_set_headless_test_properties(headless.run_script_rejects_gui_api)

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

    add_test(
      NAME headless.trace_cylinder_scene
      COMMAND "${TONATIUHPP_TEST_EXE}" --headless trace-scene "${_tonatiuhpp_cylinder_scene}" --rays 10 --seed 123456789 --no-export
    )
    tonatiuhpp_set_headless_test_properties(headless.trace_cylinder_scene)
    set_tests_properties(headless.trace_cylinder_scene PROPERTIES
      PASS_REGULAR_EXPRESSION "Trace completed\\."
      TIMEOUT 60
    )

    add_test(
      NAME headless.benchmark_cylinder_scene
      COMMAND "${CMAKE_COMMAND}"
        "-DTEST_EXECUTABLE=${TONATIUHPP_TEST_EXE}"
        "-DTEST_WORKING_DIRECTORY=${TONATIUHPP_TEST_WORKING_DIR}"
        "-DSCENE_FILE=${_tonatiuhpp_cylinder_scene}"
        "-DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}/headless_benchmark_cylinder"
        -P "${CMAKE_CURRENT_LIST_DIR}/cmake/run_headless_benchmark_smoke.cmake"
    )
    tonatiuhpp_set_headless_test_properties(headless.benchmark_cylinder_scene)
    set_tests_properties(headless.benchmark_cylinder_scene PROPERTIES
      TIMEOUT 60
    )

    # Check cross-process repeatability across three fixed 10,000-ray
    # chunks, without altering benchmark reference data or runtime physics.
    add_test(
      NAME headless.benchmark_repeatability
      COMMAND "${CMAKE_COMMAND}"
        "-DTEST_EXECUTABLE=${TONATIUHPP_TEST_EXE}"
        "-DTEST_WORKING_DIRECTORY=${TONATIUHPP_TEST_WORKING_DIR}"
        "-DSCENE_FILE=${_tonatiuhpp_cylinder_scene}"
        "-DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}/headless_benchmark_repeatability"
        -P "${CMAKE_CURRENT_LIST_DIR}/cmake/run_headless_benchmark_repeatability.cmake"
    )
    tonatiuhpp_set_headless_test_properties(headless.benchmark_repeatability)
    set_tests_properties(headless.benchmark_repeatability PROPERTIES TIMEOUT 120)

    add_test(
      NAME headless.run_script_cylinder_scene
      COMMAND "${CMAKE_COMMAND}"
        "-DTEST_EXECUTABLE=${TONATIUHPP_TEST_EXE}"
        "-DTEST_WORKING_DIRECTORY=${TONATIUHPP_TEST_WORKING_DIR}"
        "-DSCENE_FILE=${_tonatiuhpp_cylinder_scene}"
        "-DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}/headless_run_script_cylinder"
        -P "${CMAKE_CURRENT_LIST_DIR}/cmake/run_headless_script_smoke.cmake"
    )
    tonatiuhpp_set_headless_test_properties(headless.run_script_cylinder_scene)
    set_tests_properties(headless.run_script_cylinder_scene PROPERTIES
      TIMEOUT 60
    )
  endif()

  # Use the original frozen scene and benchmark-v2 receiver parameters.
  set(_heliostat_scene "${CMAKE_CURRENT_LIST_DIR}/../examples/benchmarks/benchmark_heliostat_field_v1.tnhpp")
  set(_heliostat_config "${CMAKE_CURRENT_LIST_DIR}/../examples/benchmarks/benchmark_config_v2.example.json")
  if(EXISTS "${_heliostat_scene}" AND EXISTS "${_heliostat_config}")
    add_test(
      NAME headless.validate_heliostat_scene
      COMMAND "${TONATIUHPP_TEST_EXE}" --headless validate-scene "${_heliostat_scene}"
    )
    tonatiuhpp_set_headless_test_properties(headless.validate_heliostat_scene)
    set_tests_properties(headless.validate_heliostat_scene PROPERTIES
      PASS_REGULAR_EXPRESSION "Scene validation succeeded"
      TIMEOUT 180
    )

    add_test(
      NAME headless.benchmark_heliostat_10k
      COMMAND "${CMAKE_COMMAND}"
        "-DTEST_EXECUTABLE=${TONATIUHPP_TEST_EXE}"
        "-DTEST_WORKING_DIRECTORY=${TONATIUHPP_TEST_WORKING_DIR}"
        "-DSOURCE_CONFIG=${_heliostat_config}"
        "-DSCENE_FILE=${_heliostat_scene}"
        "-DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}/headless_benchmark_heliostat"
        "-DMODE=smoke"
        -P "${CMAKE_CURRENT_LIST_DIR}/cmake/run_heliostat_benchmark_m5.cmake"
    )
    tonatiuhpp_set_headless_test_properties(headless.benchmark_heliostat_10k)
    set_tests_properties(headless.benchmark_heliostat_10k PROPERTIES TIMEOUT 360)

    # Within-platform scientific regression on every supported OS: independent
    # processes, without requiring cross-platform floating-point equality.
    add_test(
      NAME headless.benchmark_heliostat_1m_repeatability
      COMMAND "${CMAKE_COMMAND}"
        "-DTEST_EXECUTABLE=${TONATIUHPP_TEST_EXE}"
        "-DTEST_WORKING_DIRECTORY=${TONATIUHPP_TEST_WORKING_DIR}"
        "-DSOURCE_CONFIG=${_heliostat_config}"
        "-DSCENE_FILE=${_heliostat_scene}"
        "-DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}/headless_benchmark_heliostat"
        "-DMODE=repeatability"
        -P "${CMAKE_CURRENT_LIST_DIR}/cmake/run_heliostat_benchmark_m5.cmake"
    )
    tonatiuhpp_set_headless_test_properties(headless.benchmark_heliostat_1m_repeatability)
    set_tests_properties(headless.benchmark_heliostat_1m_repeatability PROPERTIES TIMEOUT 1200)
  endif()
endif()
