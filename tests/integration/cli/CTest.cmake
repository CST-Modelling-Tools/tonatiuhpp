# Registration included by tests/CMakeLists.txt; same CMake scope.
  # Unified M3 domain Run: two independent headless processes use one
  # widget-free executor. Generate a native GUI comparison fixture too.
  add_test(
    NAME headless.m3_shared_run_parity
    COMMAND "${CMAKE_COMMAND}"
      "-DTEST_EXECUTABLE=${TONATIUHPP_TEST_EXE}"
      "-DTEST_SCRIPT=${TONATIUHPP_TESTS_ROOT}/scripts/m3_shared_run_parity.tnhpps.in"
      "-DTEST_SCENE=${TONATIUHPP_TESTS_ROOT}/../examples/benchmarks/cylinder.tnhpp"
      "-DCOMPARATOR=${TONATIUHPP_TESTS_ROOT}/scientific/compare_photon_exports.py"
      "-DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}/m3_shared_run_parity"
      "-DTEST_BUILD_ROOT=${CMAKE_CURRENT_BINARY_DIR}"
      -P "${TONATIUHPP_TESTS_ROOT}/cmake/run_m3_shared_run_parity.cmake"
  )
  tonatiuhpp_set_headless_test_properties(headless.m3_shared_run_parity)
  set_tests_properties(headless.m3_shared_run_parity PROPERTIES
    LABELS "headless;smoke;scientific;equivalence" TIMEOUT 280)

  add_test(
    NAME headless.help
    COMMAND "${TONATIUHPP_TEST_EXE}"  --help
  )
  tonatiuhpp_set_headless_test_properties(headless.help)
  set_tests_properties(headless.help PROPERTIES
    PASS_REGULAR_EXPRESSION "Tonatiuh\\+\\+ headless mode"
  )

  add_test(
    NAME headless.invalid_arguments
    COMMAND "${CMAKE_COMMAND}"
      "-DTEST_EXECUTABLE=${TONATIUHPP_TEST_EXE}"
      "-DTEST_ARGUMENTS=not-a-command"
      "-DEXPECTED_EXIT_CODE=2"
      "-DTEST_WORKING_DIRECTORY=${TONATIUHPP_TEST_WORKING_DIR}"
      -P "${TONATIUHPP_TESTS_ROOT}/cmake/expect_exit_code.cmake"
  )
  tonatiuhpp_set_headless_test_properties(headless.invalid_arguments)

  add_test(
    NAME headless.run_script_missing_path
    COMMAND "${CMAKE_COMMAND}"
      "-DTEST_EXECUTABLE=${TONATIUHPP_TEST_EXE}"
      "-DTEST_ARGUMENTS=run-script"
      "-DEXPECTED_EXIT_CODE=2"
      "-DTEST_WORKING_DIRECTORY=${TONATIUHPP_TEST_WORKING_DIR}"
      -P "${TONATIUHPP_TESTS_ROOT}/cmake/expect_exit_code.cmake"
  )
  tonatiuhpp_set_headless_test_properties(headless.run_script_missing_path)

  add_test(
    NAME headless.run_script_missing_file
    COMMAND "${CMAKE_COMMAND}"
      "-DTEST_EXECUTABLE=${TONATIUHPP_TEST_EXE}"
      "-DTEST_ARGUMENTS=run-script|${CMAKE_CURRENT_BINARY_DIR}/missing_headless_script.tnhpps"
      "-DEXPECTED_EXIT_CODE=1"
      "-DTEST_WORKING_DIRECTORY=${TONATIUHPP_TEST_WORKING_DIR}"
      -P "${TONATIUHPP_TESTS_ROOT}/cmake/expect_exit_code.cmake"
  )
  tonatiuhpp_set_headless_test_properties(headless.run_script_missing_file)

  add_test(
    NAME headless.run_script_rejects_gui_api
    COMMAND "${CMAKE_COMMAND}"
      "-DTEST_EXECUTABLE=${TONATIUHPP_TEST_EXE}"
      "-DTEST_WORKING_DIRECTORY=${TONATIUHPP_TEST_WORKING_DIR}"
      "-DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}/headless_run_script_errors"
      "-DTEST_BUILD_ROOT=${CMAKE_CURRENT_BINARY_DIR}"
      -P "${TONATIUHPP_TESTS_ROOT}/cmake/run_headless_script_error_smoke.cmake"
  )
  tonatiuhpp_set_headless_test_properties(headless.run_script_rejects_gui_api)

  add_test(
    NAME headless.run_script_scene_document
    COMMAND "${CMAKE_COMMAND}"
      "-DTEST_EXECUTABLE=${TONATIUHPP_TEST_EXE}"
      "-DTEST_WORKING_DIRECTORY=${TONATIUHPP_TEST_WORKING_DIR}"
      "-DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}/headless_scene_document"
      "-DTEST_BUILD_ROOT=${CMAKE_CURRENT_BINARY_DIR}"
      -P "${TONATIUHPP_TESTS_ROOT}/cmake/run_headless_scene_document_smoke.cmake"
  )
  tonatiuhpp_set_headless_test_properties(headless.run_script_scene_document)

  # M3.3: verify that headless script JSON and DataObject paths are relative
  # to the script, even if the process starts from a different directory.
  add_test(
    NAME headless.m3_script_relative_io
    COMMAND "${CMAKE_COMMAND}"
      "-DTEST_EXECUTABLE=${TONATIUHPP_TEST_EXE}"
      "-DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}/m3_script_relative_io"
      "-DTEST_BUILD_ROOT=${CMAKE_CURRENT_BINARY_DIR}"
      -P "${TONATIUHPP_TESTS_ROOT}/cmake/run_m3_script_relative_io.cmake"
  )
  tonatiuhpp_set_headless_test_properties(headless.m3_script_relative_io)
  set_tests_properties(headless.m3_script_relative_io PROPERTIES TIMEOUT 45)

  # Run two small, unmodified tracked examples. They construct scenes but do
  # not trace rays or access sidecar CSVs. ShapeFunctionXYZ must be built.
  add_test(
    NAME headless.run_script_cpc_curved
    COMMAND "${TONATIUHPP_TEST_EXE}"  run-script
      "${TONATIUHPP_TESTS_ROOT}/../examples/devices/CPC/CPC-rotated.tnhpps"
  )
  tonatiuhpp_set_headless_test_properties(headless.run_script_cpc_curved)
  set_tests_properties(headless.run_script_cpc_curved PROPERTIES TIMEOUT 30)

  add_test(
    NAME headless.run_script_cpc_faceted
    COMMAND "${TONATIUHPP_TEST_EXE}"  run-script
      "${TONATIUHPP_TESTS_ROOT}/../examples/devices/CPC/CPC-rotated-step.tnhpps"
  )
  tonatiuhpp_set_headless_test_properties(headless.run_script_cpc_faceted)
  set_tests_properties(headless.run_script_cpc_faceted PROPERTIES TIMEOUT 30)

  # M3 exit Phase 2: same non-destructive sun-position script as the native GUI.
  add_test(
    NAME headless.m3_exit_phase2_sun_position
    COMMAND "${TONATIUHPP_TEST_EXE}"  run-script
      "${TONATIUHPP_TESTS_ROOT}/scripts/m3_exit_phase2_sun_smoke.tnhpps"
  )
  tonatiuhpp_set_headless_test_properties(headless.m3_exit_phase2_sun_position)
  set_tests_properties(headless.m3_exit_phase2_sun_position PROPERTIES TIMEOUT 30)

  # Phase 3A: bounded shared scene-field edit and 64x48 grid parity.
  # The runner copies the fixture to build/ and writes a headless baseline;
  # GUI acceptance reuses the copied, unchanged script, not source-tree CSVs.
  add_test(
    NAME headless.m3_exit_phase3a_field_grid
    COMMAND "${CMAKE_COMMAND}"
      "-DTEST_EXECUTABLE=${TONATIUHPP_TEST_EXE}"
      "-DTEST_SCRIPT=${TONATIUHPP_TESTS_ROOT}/scripts/m3_exit_phase3a_field_grid_smoke.tnhpps"
      "-DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}/m3_exit_phase3a_field_grid"
      "-DTEST_BUILD_ROOT=${CMAKE_CURRENT_BINARY_DIR}"
      -P "${TONATIUHPP_TESTS_ROOT}/cmake/run_m3_exit_phase3a_field_grid.cmake"
  )
  tonatiuhpp_set_headless_test_properties(headless.m3_exit_phase3a_field_grid)
  set_tests_properties(headless.m3_exit_phase3a_field_grid PROPERTIES
    LABELS "headless;smoke;scientific;equivalence" TIMEOUT 90)

  # Phase 3B2B2A: real production File append and cumulative power parity.
  # Uses a disposable build/tests directory and the existing A0 comparator.
  add_test(
    NAME headless.m3_exit_phase3b2b2a_append
    COMMAND "${CMAKE_COMMAND}"
      "-DTEST_EXECUTABLE=${TONATIUHPP_TEST_EXE}"
      "-DTEST_SCRIPT=${TONATIUHPP_TESTS_ROOT}/scripts/m3_exit_phase3b2b2_append.tnhpps.in"
      "-DTEST_SCENE=${TONATIUHPP_TESTS_ROOT}/../examples/benchmarks/cylinder.tnhpp"
      "-DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}/m3_exit_phase3b2b2a_append"
      "-DTEST_BUILD_ROOT=${CMAKE_CURRENT_BINARY_DIR}"
      "-DAPPEND_VERIFIER=${TONATIUHPP_TESTS_ROOT}/scientific/verify_m3_exit_phase3b2b2_append.py"
      "-DCOMPARATOR=${TONATIUHPP_TESTS_ROOT}/scientific/compare_photon_exports.py"
      -P "${TONATIUHPP_TESTS_ROOT}/cmake/run_m3_exit_phase3b2b2a_append.cmake"
  )
  tonatiuhpp_set_headless_test_properties(headless.m3_exit_phase3b2b2a_append)
  set_tests_properties(headless.m3_exit_phase3b2b2a_append PROPERTIES
    LABELS "headless;smoke;scientific;equivalence" TIMEOUT 180)

  # Phase 3B2B1: real, bounded one-shot scripted File photon exporter.
  # Tests repeatability, selected-surface filtering and no-overwrite failure.
  add_test(
    NAME headless.m3_exit_phase3b2b1_run_file
    COMMAND "${CMAKE_COMMAND}"
      "-DTEST_EXECUTABLE=${TONATIUHPP_TEST_EXE}"
      "-DTEST_SCRIPT=${TONATIUHPP_TESTS_ROOT}/scripts/m3_exit_phase3b2b1_run_file.tnhpps.in"
      "-DTEST_SCENE=${TONATIUHPP_TESTS_ROOT}/../examples/benchmarks/cylinder.tnhpp"
      "-DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}/m3_exit_phase3b2b1_run_file"
      "-DTEST_BUILD_ROOT=${CMAKE_CURRENT_BINARY_DIR}"
      "-DCOMPARATOR=${TONATIUHPP_TESTS_ROOT}/scientific/compare_photon_exports.py"
      -P "${TONATIUHPP_TESTS_ROOT}/cmake/run_m3_exit_phase3b2b1_file.cmake"
  )
  tonatiuhpp_set_headless_test_properties(headless.m3_exit_phase3b2b1_run_file)
  set_tests_properties(headless.m3_exit_phase3b2b1_run_file PROPERTIES
    LABELS "headless;smoke;scientific;equivalence" TIMEOUT 120)

  # Actual no-export ray-tracing operation with a strictly small ray budget.
  add_test(
    NAME headless.run_script_interception_bounded
    COMMAND "${CMAKE_COMMAND}"
      "-DTEST_EXECUTABLE=${TONATIUHPP_TEST_EXE}"
      "-DTEST_WORKING_DIRECTORY=${TONATIUHPP_TEST_WORKING_DIR}"
      "-DTEST_SCRIPT=${TONATIUHPP_TESTS_ROOT}/scripts/m3_2_interception_bounded.tnhpps"
      "-DERROR_SCRIPT=${TONATIUHPP_TESTS_ROOT}/scripts/m3_2_interception_error.tnhpps"
      -P "${TONATIUHPP_TESTS_ROOT}/cmake/run_headless_interception_smoke.cmake"
  )
  tonatiuhpp_set_headless_test_properties(headless.run_script_interception_bounded)
  set_tests_properties(headless.run_script_interception_bounded PROPERTIES TIMEOUT 75)

  set(_tonatiuhpp_cylinder_scene "${TONATIUHPP_TESTS_ROOT}/../examples/benchmarks/cylinder.tnhpp")
  if(EXISTS "${_tonatiuhpp_cylinder_scene}")
    # M3.3: one real script-controlled session must be cancellable, never
    # report accepted partial science, and permit a fresh completed trace.
    add_test(
      NAME headless.m3_script_cancel_recover
      COMMAND "${CMAKE_COMMAND}"
        "-DTEST_EXECUTABLE=${TONATIUHPP_TEST_EXE}"
        "-DTEST_WORKING_DIRECTORY=${TONATIUHPP_TEST_WORKING_DIR}"
        "-DSCENE_FILE=${_tonatiuhpp_cylinder_scene}"
        "-DSOURCE_DIR=${TONATIUHPP_TESTS_ROOT}"
        "-DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}/headless_m3_cancel_recover"
      "-DTEST_BUILD_ROOT=${CMAKE_CURRENT_BINARY_DIR}"
        -P "${TONATIUHPP_TESTS_ROOT}/cmake/run_m3_script_cancel_smoke.cmake"
    )
    tonatiuhpp_set_headless_test_properties(headless.m3_script_cancel_recover)
    set_tests_properties(headless.m3_script_cancel_recover PROPERTIES TIMEOUT 120)

    add_test(
      NAME headless.validate_cylinder_scene
      COMMAND "${TONATIUHPP_TEST_EXE}"  validate-scene "${_tonatiuhpp_cylinder_scene}"
    )
    tonatiuhpp_set_headless_test_properties(headless.validate_cylinder_scene)
    set_tests_properties(headless.validate_cylinder_scene PROPERTIES
      PASS_REGULAR_EXPRESSION "Scene validation succeeded"
    )

    add_test(
      NAME headless.trace_cylinder_scene
      COMMAND "${TONATIUHPP_TEST_EXE}"  trace-scene "${_tonatiuhpp_cylinder_scene}" --rays 10 --seed 123456789 --no-export
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
      "-DTEST_BUILD_ROOT=${CMAKE_CURRENT_BINARY_DIR}"
        -P "${TONATIUHPP_TESTS_ROOT}/cmake/run_headless_benchmark_smoke.cmake"
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
      "-DTEST_BUILD_ROOT=${CMAKE_CURRENT_BINARY_DIR}"
        -P "${TONATIUHPP_TESTS_ROOT}/cmake/run_headless_benchmark_repeatability.cmake"
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
      "-DTEST_BUILD_ROOT=${CMAKE_CURRENT_BINARY_DIR}"
        -P "${TONATIUHPP_TESTS_ROOT}/cmake/run_headless_script_smoke.cmake"
    )
    tonatiuhpp_set_headless_test_properties(headless.run_script_cylinder_scene)
    set_tests_properties(headless.run_script_cylinder_scene PROPERTIES
      TIMEOUT 60
    )
  endif()

  # Real CLI entry point on the tracked fixture. Keep the default case
  # untouched while verifying explicit opt-in reaches scene preparation.
  set(_tracker_scene "${TONATIUHPP_TESTS_ROOT}/scientific/fixtures/one-axis-tracker.tnhpp")
