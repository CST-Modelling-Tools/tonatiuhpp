# Registration included by tests/CMakeLists.txt; same CMake scope.
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
        -P "${TONATIUHPP_TESTS_ROOT}/cmake/run_native_tracker_signatures.cmake"
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
        "-DSOURCE_DIR=${TONATIUHPP_TESTS_ROOT}"
        -P "${TONATIUHPP_TESTS_ROOT}/cmake/run_photon_export_scientific_equivalence.cmake"
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
        "-DTEST_SCRIPT=${TONATIUHPP_TESTS_ROOT}/scripts/m4_cli_file_export_parity.tnhpps.in"
        "-DCOMPARATOR=${TONATIUHPP_TESTS_ROOT}/scientific/compare_photon_exports.py"
        "-DPYTHON_EXECUTABLE=${Python3_EXECUTABLE}"
        "-DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}/m4_cli_file_export_parity"
        "-DTEST_BUILD_ROOT=${CMAKE_CURRENT_BINARY_DIR}"
        -P "${TONATIUHPP_TESTS_ROOT}/cmake/run_m4_cli_file_export_parity.cmake"
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
        "-DTEST_SCRIPT=${TONATIUHPP_TESTS_ROOT}/scripts/m4_extended_file_export.tnhpps.in"
        "-DTEST_SCENE=${_tonatiuhpp_cylinder_scene}"
        "-DCOMPARATOR=${TONATIUHPP_TESTS_ROOT}/scientific/compare_m4_extended_exports.py"
        "-DPYTHON_EXECUTABLE=${Python3_EXECUTABLE}"
        "-DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}/m4_extended_file_export"
        "-DTEST_BUILD_ROOT=${CMAKE_CURRENT_BINARY_DIR}"
        "-DTEST_WORKING_DIRECTORY=${TONATIUHPP_TEST_WORKING_DIR}"
        -P "${TONATIUHPP_TESTS_ROOT}/cmake/run_m4_extended_file_export.cmake"
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
        "-DSOURCE_DIR=${TONATIUHPP_TESTS_ROOT}"
        "-DUPDATE_TRACKERS=ON"
        -P "${TONATIUHPP_TESTS_ROOT}/cmake/run_photon_export_scientific_equivalence.cmake"
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
    set(_ray_history_specular_scene "${TONATIUHPP_TESTS_ROOT}/scientific/fixtures/specular-two-reflections.tnhpp")
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
            "-DSOURCE_DIR=${TONATIUHPP_TESTS_ROOT}"
            "-DUPDATE_TRACKERS=${_ray_history_tracker}"
            -P "${TONATIUHPP_TESTS_ROOT}/cmake/run_native_ray_history_equivalence.cmake"
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
  set(_heliostat_scene "${TONATIUHPP_TESTS_ROOT}/../examples/benchmarks/benchmark_heliostat_field_v1.tnhpp")
  set(_heliostat_config "${TONATIUHPP_TESTS_ROOT}/../examples/benchmarks/benchmark_config_v2.example.json")
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
        -P "${TONATIUHPP_TESTS_ROOT}/cmake/run_heliostat_benchmark_m5.cmake"
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
        -P "${TONATIUHPP_TESTS_ROOT}/cmake/run_heliostat_benchmark_m5.cmake"
    )
    tonatiuhpp_set_headless_test_properties(headless.benchmark_heliostat_1m_repeatability)
    set_tests_properties(headless.benchmark_heliostat_1m_repeatability PROPERTIES TIMEOUT 1200)
  endif()
