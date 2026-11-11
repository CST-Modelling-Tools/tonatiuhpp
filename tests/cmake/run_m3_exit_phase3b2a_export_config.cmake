if(NOT DEFINED TEST_EXECUTABLE OR NOT EXISTS "${TEST_EXECUTABLE}")
  message(FATAL_ERROR "Phase 3B2A: TEST_EXECUTABLE missing")
endif()
if(NOT DEFINED TEST_SCRIPT OR NOT EXISTS "${TEST_SCRIPT}")
  message(FATAL_ERROR "Phase 3B2A: TEST_SCRIPT missing")
endif()
if(NOT DEFINED OUTPUT_DIR OR OUTPUT_DIR STREQUAL "")
  message(FATAL_ERROR "Phase 3B2A: OUTPUT_DIR missing")
endif()
file(MAKE_DIRECTORY "${OUTPUT_DIR}/script")
file(COPY_FILE "${TEST_SCRIPT}"
  "${OUTPUT_DIR}/script/m3_exit_phase3b2a_export_config.tnhpps" ONLY_IF_DIFFERENT)
execute_process(
  COMMAND "${TEST_EXECUTABLE}"  run-script
    "${OUTPUT_DIR}/script/m3_exit_phase3b2a_export_config.tnhpps"
  WORKING_DIRECTORY "${OUTPUT_DIR}/script"
  RESULT_VARIABLE result
  OUTPUT_VARIABLE out
  ERROR_VARIABLE err)
if(NOT result EQUAL 0)
  message(FATAL_ERROR "Phase 3B2A headless configuration failed (${result}):\n${out}\n${err}")
endif()
if(NOT out MATCHES "M3 Phase 3B2A export-option configuration smoke passed")
  message(FATAL_ERROR "Phase 3B2A did not report its expected sentinel:\n${out}\n${err}")
endif()
message(STATUS "Phase 3B2A headless configuration: passed (no trace/export)")
