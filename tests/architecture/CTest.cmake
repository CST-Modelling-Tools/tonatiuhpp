# Registration included by tests/CMakeLists.txt; same CMake scope.
# P2: verify real GUI and CLI names, kernel identity and legacy-mode rejection.
find_package(Python3 COMPONENTS Interpreter REQUIRED)
add_test(NAME architecture.gui_cli_runtime_boundary
  COMMAND "${Python3_EXECUTABLE}"
    "${TONATIUHPP_TESTS_ROOT}/architecture/verify_p2_binaries.py"
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
        "${TONATIUHPP_TESTS_ROOT}/architecture/verify_large_export_preflight.py"
        --cli "$<TARGET_FILE:tonatiuhpp-cli>"
        --scene "${TONATIUHPP_TESTS_ROOT}/../examples/benchmarks/cylinder.tnhpp"
        --build-test-dir "${CMAKE_CURRENT_BINARY_DIR}")
set_tests_properties(headless.large_file_export_preflight PROPERTIES
    LABELS "headless;scientific;smoke;architecture" TIMEOUT 120)
