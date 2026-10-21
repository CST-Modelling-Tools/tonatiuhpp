# Real native-process regression for the M3.3 script-relative I/O contract.
if(NOT DEFINED TEST_EXECUTABLE OR NOT DEFINED OUTPUT_DIR)
  message(FATAL_ERROR "TEST_EXECUTABLE and OUTPUT_DIR are required")
endif()
file(REMOVE_RECURSE "${OUTPUT_DIR}")
file(MAKE_DIRECTORY "${OUTPUT_DIR}/scripts" "${OUTPUT_DIR}/elsewhere")
set(script_file "${OUTPUT_DIR}/scripts/relative_io.tnhpps")
file(WRITE "${script_file}" [=[
var data = new DataObject;
data.addRow('M3.3 relative IO');
if (!data.write('data.txt')) throw new Error('DataObject.write failed');
var reread = new DataObject;
if (!reread.read('data.txt') || reread.row(0) !== 'M3.3 relative IO')
    throw new Error('DataObject script-relative roundtrip failed');
if (!tn.writeJson('artifacts/result.json', {status: 'M3.3 relative IO'}))
    throw new Error('tn.writeJson failed');
print('M3.3 relative IO passed');
]=])
execute_process(
  COMMAND "${TEST_EXECUTABLE}" --headless run-script "${script_file}"
  WORKING_DIRECTORY "${OUTPUT_DIR}/elsewhere"
  RESULT_VARIABLE exit_code OUTPUT_VARIABLE stdout_text ERROR_VARIABLE stderr_text
  TIMEOUT 30
)
if(NOT "${exit_code}" STREQUAL "0")
  message(FATAL_ERROR "M3.3 script IO failed (${exit_code}): ${stderr_text} ${stdout_text}")
endif()
if(NOT stdout_text MATCHES "M3\\.3 relative IO passed")
  message(FATAL_ERROR "M3.3 success marker missing: ${stdout_text}")
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
if(NOT result_json MATCHES "M3\\.3 relative IO")
  message(FATAL_ERROR "JSON payload mismatch: ${result_json}")
endif()
message(STATUS "M3.3 script-relative DataObject and JSON output passed")
