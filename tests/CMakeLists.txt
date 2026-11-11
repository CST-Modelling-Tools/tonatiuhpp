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

if(TONATIUHPP_ENABLE_HEADLESS_SMOKE_TESTS)
  # Unified M3 domain Run: two independent headless processes use one
  # widget-free executor. Generate a native GUI comparison fixture too.
  add_test(
    NAME headless.m3_shared_run_parity
    COMMAND "${CMAKE_COMMAND}"
      "-DTEST_EXECUTABLE=${TONATIUHPP_TEST_EXE}"
      "-DTEST_SCRIPT=${CMAKE_CURRENT_LIST_DIR}/scripts/m3_shared_run_parity.tnhpps.in"
      "-DTEST_SCENE=${CMAKE_CURRENT_LIST_DIR}/../examples/benchmarks/cylinder.tnhpp"
      "-DCOMPARATOR=${CMAKE_CURRENT_LIST_DIR}/scientific/compare_photon_exports.py"
      "-DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}/m3_shared_run_parity"
      "-DTEST_BUILD_ROOT=${CMAKE_CURRENT_BINARY_DIR}"
      -P "${CMAKE_CURRENT_LIST_DIR}/cmake/run_m3_shared_run_parity.cmake"
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
      -P "${CMAKE_CURRENT_LIST_DIR}/cmake/expect_exit_code.cmake"
  )
  tonatiuhpp_set_headless_test_properties(headless.invalid_arguments)

  add_test(
    NAME headless.run_script_missing_path
    COMMAND "${CMAKE_COMMAND}"
      "-DTEST_EXECUTABLE=${TONATIUHPP_TEST_EXE}"
      "-DTEST_ARGUMENTS=run-script"
      "-DEXPECTED_EXIT_CODE=2"
      "-DTEST_WORKING_DIRECTORY=${TONATIUHPP_TEST_WORKING_DIR}"
      -P "${CMAKE_CURRENT_LIST_DIR}/cmake/expect_exit_code.cmake"
  )
  tonatiuhpp_set_headless_test_properties(headless.run_script_missing_path)

  add_test(
    NAME headless.run_script_missing_file
    COMMAND "${CMAKE_COMMAND}"
      "-DTEST_EXECUTABLE=${TONATIUHPP_TEST_EXE}"
      "-DTEST_ARGUMENTS=run-script|${CMAKE_CURRENT_BINARY_DIR}/missing_headless_script.tnhpps"
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

  add_test(
    NAME headless.run_script_scene_document
    COMMAND "${CMAKE_COMMAND}"
      "-DTEST_EXECUTABLE=${TONATIUHPP_TEST_EXE}"
      "-DTEST_WORKING_DIRECTORY=${TONATIUHPP_TEST_WORKING_DIR}"
      "-DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}/headless_scene_document"
      -P "${CMAKE_CURRENT_LIST_DIR}/cmake/run_headless_scene_document_smoke.cmake"
  )
  tonatiuhpp_set_headless_test_properties(headless.run_script_scene_document)

  # M3.3: verify that headless script JSON and DataObject paths are relative
  # to the script, even if the process starts from a different directory.
  add_test(
    NAME headless.m3_script_relative_io
    COMMAND "${CMAKE_COMMAND}"
      "-DTEST_EXECUTABLE=${TONATIUHPP_TEST_EXE}"
      "-DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}/m3_script_relative_io"
      -P "${CMAKE_CURRENT_LIST_DIR}/cmake/run_m3_script_relative_io.cmake"
  )
  tonatiuhpp_set_headless_test_properties(headless.m3_script_relative_io)
  set_tests_properties(headless.m3_script_relative_io PROPERTIES TIMEOUT 45)

  # Run two small, unmodified tracked examples. They construct scenes but do
  # not trace rays or access sidecar CSVs. ShapeFunctionXYZ must be built.
  add_test(
    NAME headless.run_script_cpc_curved
    COMMAND "${TONATIUHPP_TEST_EXE}"  run-script
      "${CMAKE_CURRENT_LIST_DIR}/../examples/devices/CPC/CPC-rotated.tnhpps"
  )
  tonatiuhpp_set_headless_test_properties(headless.run_script_cpc_curved)
  set_tests_properties(headless.run_script_cpc_curved PROPERTIES TIMEOUT 30)

  add_test(
    NAME headless.run_script_cpc_faceted
    COMMAND "${TONATIUHPP_TEST_EXE}"  run-script
      "${CMAKE_CURRENT_LIST_DIR}/../examples/devices/CPC/CPC-rotated-step.tnhpps"
  )
  tonatiuhpp_set_headless_test_properties(headless.run_script_cpc_faceted)
  set_tests_properties(headless.run_script_cpc_faceted PROPERTIES TIMEOUT 30)

  # M3 exit Phase 2: same non-destructive sun-position script as the native GUI.
  add_test(
    NAME headless.m3_exit_phase2_sun_position
    COMMAND "${TONATIUHPP_TEST_EXE}"  run-script
      "${CMAKE_CURRENT_LIST_DIR}/scripts/m3_exit_phase2_sun_smoke.tnhpps"
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
      "-DTEST_SCRIPT=${CMAKE_CURRENT_LIST_DIR}/scripts/m3_exit_phase3a_field_grid_smoke.tnhpps"
      "-DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}/m3_exit_phase3a_field_grid"
      -P "${CMAKE_CURRENT_LIST_DIR}/cmake/run_m3_exit_phase3a_field_grid.cmake"
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
      "-DTEST_SCRIPT=${CMAKE_CURRENT_LIST_DIR}/scripts/m3_exit_phase3b2b2_append.tnhpps.in"
      "-DTEST_SCENE=${CMAKE_CURRENT_LIST_DIR}/../examples/benchmarks/cylinder.tnhpp"
      "-DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}/m3_exit_phase3b2b2a_append"
      "-DTEST_BUILD_ROOT=${CMAKE_CURRENT_BINARY_DIR}"
      "-DAPPEND_VERIFIER=${CMAKE_CURRENT_LIST_DIR}/scientific/verify_m3_exit_phase3b2b2_append.py"
      "-DCOMPARATOR=${CMAKE_CURRENT_LIST_DIR}/scientific/compare_photon_exports.py"
      -P "${CMAKE_CURRENT_LIST_DIR}/cmake/run_m3_exit_phase3b2b2a_append.cmake"
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
      "-DTEST_SCRIPT=${CMAKE_CURRENT_LIST_DIR}/scripts/m3_exit_phase3b2b1_run_file.tnhpps.in"
      "-DTEST_SCENE=${CMAKE_CURRENT_LIST_DIR}/../examples/benchmarks/cylinder.tnhpp"
      "-DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}/m3_exit_phase3b2b1_run_file"
      "-DTEST_BUILD_ROOT=${CMAKE_CURRENT_BINARY_DIR}"
      "-DCOMPARATOR=${CMAKE_CURRENT_LIST_DIR}/scientific/compare_photon_exports.py"
      -P "${CMAKE_CURRENT_LIST_DIR}/cmake/run_m3_exit_phase3b2b1_file.cmake"
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
      "-DTEST_SCRIPT=${CMAKE_CURRENT_LIST_DIR}/scripts/m3_2_interception_bounded.tnhpps"
      "-DERROR_SCRIPT=${CMAKE_CURRENT_LIST_DIR}/scripts/m3_2_interception_error.tnhpps"
      -P "${CMAKE_CURRENT_LIST_DIR}/cmake/run_headless_interception_smoke.cmake"
  )
  tonatiuhpp_set_headless_test_properties(headless.run_script_interception_bounded)
  set_tests_properties(headless.run_script_interception_bounded PROPERTIES TIMEOUT 75)

  set(_tonatiuhpp_cylinder_scene "${CMAKE_CURRENT_LIST_DIR}/../examples/benchmarks/cylinder.tnhpp")
  if(EXISTS "${_tonatiuhpp_cylinder_scene}")
    # M3.3: one real script-controlled session must be cancellable, never
    # report accepted partial science, and permit a fresh completed trace.
    add_test(
      NAME headless.m3_script_cancel_recover
      COMMAND "${CMAKE_COMMAND}"
        "-DTEST_EXECUTABLE=${TONATIUHPP_TEST_EXE}"
        "-DTEST_WORKING_DIRECTORY=${TONATIUHPP_TEST_WORKING_DIR}"
        "-DSCENE_FILE=${_tonatiuhpp_cylinder_scene}"
        "-DSOURCE_DIR=${CMAKE_CURRENT_LIST_DIR}"
        "-DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}/headless_m3_cancel_recover"
        -P "${CMAKE_CURRENT_LIST_DIR}/cmake/run_m3_script_cancel_smoke.cmake"
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

  # Real CLI entry point on the tracked fixture. Keep the default case
  # untouched while verifying explicit opt-in reaches scene preparation.
  set(_tracker_scene "${CMAKE_CURRENT_LIST_DIR}/scientific/fixtures/one-axis-tracker.tnhpp")
  if(EXISTS "${_tracker_scene}")
    add_test(
      NAME headless.trace_tracker_opt_in
      COMMAND "${TONATIUHPP_TEST_EXE}"  trace-scene "${_tracker_scene}"
              --rays 4096 --seed 123456789 --no-export --update-trackers
    )
    tonatiuhpp_set_headless_test_properties(headless.trace_tracker_opt_in)
    set_tests_properties(headless.trace_tracker_opt_in PROPERTIES
      PASS_REGULAR_EXPRESSION "tracker_update: enabled"
      TIMEOUT 120
    )

    add_test(
      NAME headless.trace_tracker_legacy
      COMMAND "${TONATIUHPP_TEST_EXE}"  trace-scene "${_tracker_scene}"
              --rays 4096 --seed 123456789 --no-export
    )
    tonatiuhpp_set_headless_test_properties(headless.trace_tracker_legacy)
    set_tests_properties(headless.trace_tracker_legacy PROPERTIES
      PASS_REGULAR_EXPRESSION "Trace completed\\."
      TIMEOUT 120
    )

    # Test exact-hit signatures from four independently launched real CLI
    # processes. This is not native MainWindow automation.
    add_test(
      NAME headless.trace_tracker_exact_signatures
      COMMAND "${CMAKE_COMMAND}"
        "-DTEST_EXECUTABLE=${TONATIUHPP_TEST_EXE}"
        "-DTEST_WORKING_DIRECTORY=${TONATIUHPP_TEST_WORKING_DIR}"
        "-DSCENE_FILE=${_tracker_scene}"
        "-DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}/headless_tracker_signatures"
        -P "${CMAKE_CURRENT_LIST_DIR}/cmake/run_native_tracker_signatures.cmake"
    )
    tonatiuhpp_set_headless_test_properties(headless.trace_tracker_exact_signatures)
    set_tests_properties(headless.trace_tracker_exact_signatures PROPERTIES
      LABELS "headless;smoke;scientific;equivalence"
      TIMEOUT 480
    )
  endif()

  # True native CLI photon file-export repeatability and scientific parity.
  # Keep this optional if Python is unavailable in a local developer build.
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND AND EXISTS "${_tonatiuhpp_cylinder_scene}")
    add_test(
      NAME headless.photon_export_scientific_equivalence
      COMMAND "${CMAKE_COMMAND}"
        "-DTEST_EXECUTABLE=${TONATIUHPP_TEST_EXE}"
        "-DTEST_WORKING_DIRECTORY=${TONATIUHPP_TEST_WORKING_DIR}"
        "-DSCENE_FILE=${_tonatiuhpp_cylinder_scene}"
        "-DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}/headless_photon_export"
        "-DPYTHON_EXECUTABLE=${Python3_EXECUTABLE}"
        "-DSOURCE_DIR=${CMAKE_CURRENT_LIST_DIR}"
        -P "${CMAKE_CURRENT_LIST_DIR}/cmake/run_photon_export_scientific_equivalence.cmake"
    )
    tonatiuhpp_set_headless_test_properties(headless.photon_export_scientific_equivalence)
    set_tests_properties(headless.photon_export_scientific_equivalence PROPERTIES
      LABELS "headless;smoke;scientific;equivalence"
      TIMEOUT 360
    )

    # M4.1: direct CLI File export (no A0 environment variable) must match
    # the identical true-headless script's real physical photon output.
    add_test(
      NAME headless.m4_cli_script_file_export_parity
      COMMAND "${CMAKE_COMMAND}"
        "-DTEST_EXECUTABLE=${TONATIUHPP_TEST_EXE}"
        "-DTEST_WORKING_DIRECTORY=${TONATIUHPP_TEST_WORKING_DIR}"
        "-DTEST_SCENE=${_tonatiuhpp_cylinder_scene}"
        "-DTEST_SCRIPT=${CMAKE_CURRENT_LIST_DIR}/scripts/m4_cli_file_export_parity.tnhpps.in"
        "-DCOMPARATOR=${CMAKE_CURRENT_LIST_DIR}/scientific/compare_photon_exports.py"
        "-DPYTHON_EXECUTABLE=${Python3_EXECUTABLE}"
        "-DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}/m4_cli_file_export_parity"
        "-DTEST_BUILD_ROOT=${CMAKE_CURRENT_BINARY_DIR}"
        -P "${CMAKE_CURRENT_LIST_DIR}/cmake/run_m4_cli_file_export_parity.cmake"
    )
    tonatiuhpp_set_headless_test_properties(headless.m4_cli_script_file_export_parity)
    set_tests_properties(headless.m4_cli_script_file_export_parity PROPERTIES
      LABELS "headless;smoke;scientific;equivalence" TIMEOUT 300)

    # M4 completion candidate: File segments, CLI/script parity, safe append,
    # and >200k requested rays without touching frozen A0 evidence.
    add_test(
      NAME headless.m4_extended_file_export_parity
      COMMAND "${CMAKE_COMMAND}"
        "-DTEST_EXECUTABLE=${TONATIUHPP_TEST_EXE}"
        "-DTEST_SCRIPT=${CMAKE_CURRENT_LIST_DIR}/scripts/m4_extended_file_export.tnhpps.in"
        "-DTEST_SCENE=${_tonatiuhpp_cylinder_scene}"
        "-DCOMPARATOR=${CMAKE_CURRENT_LIST_DIR}/scientific/compare_m4_extended_exports.py"
        "-DPYTHON_EXECUTABLE=${Python3_EXECUTABLE}"
        "-DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}/m4_extended_file_export"
        "-DTEST_BUILD_ROOT=${CMAKE_CURRENT_BINARY_DIR}"
        "-DTEST_WORKING_DIRECTORY=${TONATIUHPP_TEST_WORKING_DIR}"
        -P "${CMAKE_CURRENT_LIST_DIR}/cmake/run_m4_extended_file_export.cmake"
    )
    tonatiuhpp_set_headless_test_properties(headless.m4_extended_file_export_parity)
    set_tests_properties(headless.m4_extended_file_export_parity PROPERTIES
      LABELS "headless;smoke;scientific;equivalence" TIMEOUT 300)
  endif()

  # PR #20: real tracker recording/export regression, independent native
  # subprocesses. Each must apply GUI-style tracker preparation, including
  # the non-recording baseline; the older cylinder test stays unchanged.
  if(Python3_Interpreter_FOUND AND EXISTS "${_tracker_scene}")
    add_test(
      NAME headless.photon_export_tracker_equivalence
      COMMAND "${CMAKE_COMMAND}"
        "-DTEST_EXECUTABLE=${TONATIUHPP_TEST_EXE}"
        "-DTEST_WORKING_DIRECTORY=${TONATIUHPP_TEST_WORKING_DIR}"
        "-DSCENE_FILE=${_tracker_scene}"
        "-DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}/headless_tracker_photon_export"
        "-DPYTHON_EXECUTABLE=${Python3_EXECUTABLE}"
        "-DSOURCE_DIR=${CMAKE_CURRENT_LIST_DIR}"
        "-DUPDATE_TRACKERS=ON"
        -P "${CMAKE_CURRENT_LIST_DIR}/cmake/run_photon_export_scientific_equivalence.cmake"
    )
    tonatiuhpp_set_headless_test_properties(headless.photon_export_tracker_equivalence)
    set_tests_properties(headless.photon_export_tracker_equivalence PROPERTIES
      LABELS "headless;smoke;scientific;equivalence"
      TIMEOUT 360
    )
  endif()

  # A0 complete ordered per-ray history diagnostic. Each CTest launches
  # three independent native processes and compares recorded, exported, and
  # non-recorded histories. No frozen benchmark path runs this diagnostic.
  if(Python3_Interpreter_FOUND)
    set(_ray_history_specular_scene "${CMAKE_CURRENT_LIST_DIR}/scientific/fixtures/specular-two-reflections.tnhpp")
    foreach(_case IN ITEMS cylinder specular tracker)
      set(_ray_history_scene "")
      set(_ray_history_tracker OFF)
      if(_case STREQUAL "cylinder")
        set(_ray_history_scene "${_tonatiuhpp_cylinder_scene}")
      elseif(_case STREQUAL "specular")
        set(_ray_history_scene "${_ray_history_specular_scene}")
      else()
        set(_ray_history_scene "${_tracker_scene}")
        set(_ray_history_tracker ON)
      endif()
      if(EXISTS "${_ray_history_scene}")
        add_test(
          NAME "headless.ray_history_${_case}_equivalence"
          COMMAND "${CMAKE_COMMAND}"
            "-DTEST_EXECUTABLE=${TONATIUHPP_TEST_EXE}"
            "-DTEST_WORKING_DIRECTORY=${TONATIUHPP_TEST_WORKING_DIR}"
            "-DSCENE_FILE=${_ray_history_scene}"
            "-DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}/headless_ray_history_${_case}"
            "-DPYTHON_EXECUTABLE=${Python3_EXECUTABLE}"
            "-DSOURCE_DIR=${CMAKE_CURRENT_LIST_DIR}"
            "-DUPDATE_TRACKERS=${_ray_history_tracker}"
            -P "${CMAKE_CURRENT_LIST_DIR}/cmake/run_native_ray_history_equivalence.cmake"
        )
        tonatiuhpp_set_headless_test_properties("headless.ray_history_${_case}_equivalence")
        set_tests_properties("headless.ray_history_${_case}_equivalence" PROPERTIES
          LABELS "headless;smoke;scientific;equivalence"
          TIMEOUT 480
        )
      endif()
    endforeach()
  endif()

  # Use the original frozen scene and benchmark-v2 receiver parameters.
  set(_heliostat_scene "${CMAKE_CURRENT_LIST_DIR}/../examples/benchmarks/benchmark_heliostat_field_v1.tnhpp")
  set(_heliostat_config "${CMAKE_CURRENT_LIST_DIR}/../examples/benchmarks/benchmark_config_v2.example.json")
  if(EXISTS "${_heliostat_scene}" AND EXISTS "${_heliostat_config}")
    add_test(
      NAME headless.validate_heliostat_scene
      COMMAND "${TONATIUHPP_TEST_EXE}"  validate-scene "${_heliostat_scene}"
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

# Two independent CLI processes exercise fixed-seed hits/histories, scripts,
# Flux Analysis, cancellation, plugin loading and File export. The separate
# GUI-mode equivalence remains covered by scientific GoogleTests and manual GUI.
# The 500-million-ray benchmark is expressly OUTSIDE this test.
if(BUILD_TESTING AND TARGET tonatiuhpp-cli)
    find_package(Python3 COMPONENTS Interpreter REQUIRED)
    add_test(NAME headless.cli_independent_process_equivalence
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_LIST_DIR}/m5/m5_2_adapter_acceptance.py"
            --gui "$<TARGET_FILE:tonatiuhpp-gui>"
            --cli "$<TARGET_FILE:tonatiuhpp-cli>"
            --source-root "${CMAKE_CURRENT_LIST_DIR}/.."
            --output-root "${CMAKE_CURRENT_BINARY_DIR}/m5_2_evidence"
            --mode build)
    tonatiuhpp_set_headless_test_properties(headless.cli_independent_process_equivalence)
    set_tests_properties(headless.cli_independent_process_equivalence PROPERTIES
        LABELS "headless;smoke;scientific;equivalence" TIMEOUT 900)
endif()

# P2: verify real GUI and CLI names, kernel identity and legacy-mode rejection.
find_package(Python3 COMPONENTS Interpreter REQUIRED)
add_test(NAME architecture.gui_cli_runtime_boundary
  COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_LIST_DIR}/architecture/verify_p2_binaries.py"
    --gui "$<TARGET_FILE:tonatiuhpp-gui>"
    --cli "$<TARGET_FILE:tonatiuhpp-cli>"
    --kernel "$<TARGET_FILE:TonatiuhppKernel>"
    --libraries "$<TARGET_FILE:TonatiuhppLibraries>"
    --presentation "$<TARGET_FILE:TonatiuhppGuiPresentation>")
set_tests_properties(architecture.gui_cli_runtime_boundary PROPERTIES
    LABELS "architecture;smoke" TIMEOUT 30)

# File photon export must accept >10M rays in its CLI and scripted Run
# validation paths. Both operations fail safely before any ray is traced.
# 500-million-ray benchmark execution is expressly forbidden in this test.
add_test(NAME headless.large_file_export_preflight
    COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_LIST_DIR}/architecture/verify_large_export_preflight.py"
        --cli "$<TARGET_FILE:tonatiuhpp-cli>"
        --scene "${CMAKE_CURRENT_LIST_DIR}/../examples/benchmarks/cylinder.tnhpp"
        --build-test-dir "${CMAKE_CURRENT_BINARY_DIR}")
set_tests_properties(headless.large_file_export_preflight PROPERTIES
    LABELS "headless;scientific;smoke;architecture" TIMEOUT 120)
