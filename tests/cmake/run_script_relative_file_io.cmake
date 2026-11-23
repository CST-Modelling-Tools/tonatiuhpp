# Real native-process regression for the M3.3 script-relative I/O contract.
if(NOT DEFINED TEST_EXECUTABLE OR NOT DEFINED OUTPUT_DIR)
  message(FATAL_ERROR "TEST_EXECUTABLE and OUTPUT_DIR are required")
endif()
include("${CMAKE_CURRENT_LIST_DIR}/PrepareTestOutput.cmake")
tonatiuhpp_prepare_test_output("${OUTPUT_DIR}" "${TEST_BUILD_ROOT}" "script_relative_file_io")
file(MAKE_DIRECTORY "${OUTPUT_DIR}/scripts" "${OUTPUT_DIR}/elsewhere")
set(script_file "${OUTPUT_DIR}/scripts/relative_io.tnhpps")
file(WRITE "${script_file}" [=[
var data = new DataObject;
data.addRow('Script relative IO');
if (!data.write('data.txt')) throw new Error('DataObject.write failed');
var reread = new DataObject;
if (!reread.read('data.txt') || reread.row(0) !== 'Script relative IO')
    throw new Error('DataObject script-relative roundtrip failed');
if (!tn.writeJson('artifacts/result.json', {status: 'Script relative IO'}))
    throw new Error('tn.writeJson failed');
print('Script relative IO passed');
]=])
execute_process(
  COMMAND "${TEST_EXECUTABLE}"  run-script "${script_file}"
  WORKING_DIRECTORY "${OUTPUT_DIR}/elsewhere"
  RESULT_VARIABLE exit_code OUTPUT_VARIABLE stdout_text ERROR_VARIABLE stderr_text
  TIMEOUT 30
)
if(NOT "${exit_code}" STREQUAL "0")
  message(FATAL_ERROR "Script relative IO failed (${exit_code}): ${stderr_text} ${stdout_text}")
endif()
if(NOT stdout_text MATCHES "Script relative IO passed")
  message(FATAL_ERROR "Script relative I/O success marker missing: ${stdout_text}")
endif()
foreach(path IN ITEMS "${OUTPUT_DIR}/scripts/data.txt" "${OUTPUT_DIR}/scripts/artifacts/result.json")
  if(NOT EXISTS "${path}")
    message(FATAL_ERROR "Expected script-relative artifact missing: ${path}")
  endif()
endforeach()
foreach(path IN ITEMS "${OUTPUT_DIR}/elsewhere/data.txt" "${OUTPUT_DIR}/elsewhere/artifacts/result.json")
  if(EXISTS "${path}")
    message(FATAL_ERROR "Unexpected working-directory output: ${path}")
  endif()
endforeach()
file(READ "${OUTPUT_DIR}/scripts/artifacts/result.json" result_json)
if(NOT result_json MATCHES "Script relative IO")
  message(FATAL_ERROR "JSON payload mismatch: ${result_json}")
endif()
message(STATUS "Script-relative DataObject and JSON output passed")
