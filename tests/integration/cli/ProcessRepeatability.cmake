# Registration included by tests/CMakeLists.txt; same CMake scope.
# Two independent CLI processes exercise fixed-seed hits/histories, scripts,
# Flux Analysis, cancellation, plugin loading and File export. The separate
# GUI-mode equivalence remains covered by scientific GoogleTests and manual GUI.
# The 500-million-ray benchmark is expressly OUTSIDE this test.
if(BUILD_TESTING AND TARGET tonatiuhpp-cli)
    find_package(Python3 COMPONENTS Interpreter REQUIRED)
    add_test(NAME headless.cli_independent_process_equivalence
        COMMAND "${Python3_EXECUTABLE}"
            "${TONATIUHPP_TESTS_ROOT}/integration/cli/cli_process_repeatability.py"
            --gui "$<TARGET_FILE:tonatiuhpp-gui>"
            --cli "$<TARGET_FILE:tonatiuhpp-cli>"
            --source-root "${TONATIUHPP_TESTS_ROOT}/.."
            --output-root "${CMAKE_CURRENT_BINARY_DIR}/m5_2_evidence"
            --mode build)
    tonatiuhpp_set_headless_test_properties(headless.cli_independent_process_equivalence)
    set_tests_properties(headless.cli_independent_process_equivalence PROPERTIES
        LABELS "headless;smoke;scientific;equivalence" TIMEOUT 900)
endif()
