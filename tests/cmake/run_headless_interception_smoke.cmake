# A bounded true-headless interception exercise. Deliberately do not run the
# eight ray-heavy tracked interpolation/interception examples.
if(NOT DEFINED TEST_EXECUTABLE OR NOT DEFINED TEST_SCRIPT OR NOT DEFINED ERROR_SCRIPT)
  message(FATAL_ERROR "Executable and two script paths are required")
endif()
execute_process(
  COMMAND "${TEST_EXECUTABLE}" --headless run-script "${TEST_SCRIPT}"
  WORKING_DIRECTORY "${TEST_WORKING_DIRECTORY}"
  RESULT_VARIABLE ok_exit OUTPUT_VARIABLE ok_output ERROR_VARIABLE ok_error
  TIMEOUT 45
)
if(NOT "${ok_exit}" STREQUAL "0")
  message(FATAL_ERROR "Bounded interception failed (${ok_exit}): ${ok_error} ${ok_output}")
endif()
if(NOT ok_output MATCHES [=[M3\.2 bounded interception smoke passed]=])
  message(FATAL_ERROR "Bounded interception success marker missing: ${ok_output}")
endif()
execute_process(
  COMMAND "${TEST_EXECUTABLE}" --headless run-script "${ERROR_SCRIPT}"
  WORKING_DIRECTORY "${TEST_WORKING_DIRECTORY}"
  RESULT_VARIABLE bad_exit OUTPUT_VARIABLE bad_output ERROR_VARIABLE bad_error
  TIMEOUT 15
)
if(NOT "${bad_exit}" STREQUAL "1")
  message(FATAL_ERROR "Bad target should exit 1 (got ${bad_exit}): ${bad_error} ${bad_output}")
endif()
if(NOT bad_error MATCHES [=[m3_2_interception_error\.tnhpps:4]=])
  message(FATAL_ERROR "Bad target should report user script line 4: ${bad_error}")
endif()
message(STATUS "Bounded no-export interception and source-line errors passed")
