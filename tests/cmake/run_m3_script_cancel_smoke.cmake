# M3.3 script cancellation: same real no-export session service on native CLI.
foreach(required IN ITEMS TEST_EXECUTABLE TEST_WORKING_DIRECTORY SCENE_FILE SOURCE_DIR OUTPUT_DIR)
  if(NOT DEFINED ${required} OR "${${required}}" STREQUAL "")
    message(FATAL_ERROR "Required variable is missing: ${required}")
  endif()
endforeach()
if(NOT EXISTS "${TEST_EXECUTABLE}" OR NOT EXISTS "${SCENE_FILE}")
  message(FATAL_ERROR "Executable or scene file missing")
endif()
set(scripts "${OUTPUT_DIR}/scripts with spaces")
set(foreign "${OUTPUT_DIR}/foreign cwd")
file(MAKE_DIRECTORY "${scripts}" "${foreign}")
file(COPY "${SCENE_FILE}" DESTINATION "${scripts}")
foreach(name IN ITEMS m3_3_cancel_recover m3_3_cancel_inactive m3_3_cancel_invalid m3_3_cancel_unfinished)
  configure_file("${SOURCE_DIR}/scripts/${name}.tnhpps" "${scripts}/${name}.tnhpps" COPYONLY)
endforeach()

function(run_case name expected_code expected_text)
  execute_process(
    COMMAND "${TEST_EXECUTABLE}" --headless run-script "${scripts}/${name}.tnhpps"
    WORKING_DIRECTORY "${foreign}"
    RESULT_VARIABLE rc OUTPUT_VARIABLE stdout ERROR_VARIABLE stderr
    TIMEOUT 60
  )
  if(NOT "${rc}" STREQUAL "${expected_code}")
    message(FATAL_ERROR "${name}: expected exit ${expected_code}, actual '${rc}'.\nSTDOUT: ${stdout}\nSTDERR: ${stderr}")
  endif()
  if("${expected_code}" STREQUAL "0")
    string(FIND "${stdout}" "${expected_text}" marker)
  else()
    string(FIND "${stderr}" "${expected_text}" marker)
    string(FIND "${stderr}" "${name}.tnhpps:2" line)
    if(line EQUAL -1)
      message(FATAL_ERROR "${name}: native error did not identify the user source at line 2.\n${stderr}")
    endif()
  endif()
  if(marker EQUAL -1)
    message(FATAL_ERROR "${name}: expected message '${expected_text}' missing.\nSTDOUT: ${stdout}\nSTDERR: ${stderr}")
  endif()
  message(STATUS "PASS ${name} (exit ${rc})")
endfunction()

run_case(m3_3_cancel_recover 0 "M3.3 scripted cancellation/recovery PASSED:")
run_case(m3_3_cancel_inactive 1 "tn.cancelTrace failed: no active script trace")
run_case(m3_3_cancel_invalid 1 "options.noExport must be true")
execute_process(
  COMMAND "${TEST_EXECUTABLE}" --headless run-script "${scripts}/m3_3_cancel_unfinished.tnhpps"
  WORKING_DIRECTORY "${foreign}"
  RESULT_VARIABLE orphan_code OUTPUT_VARIABLE orphan_out ERROR_VARIABLE orphan_err
  TIMEOUT 60
)
if(NOT "${orphan_code}" STREQUAL "1")
  message(FATAL_ERROR "Unfinished session was not rejected (exit '${orphan_code}'): ${orphan_out} ${orphan_err}")
endif()
string(FIND "${orphan_err}" "unfinished trace was cancelled and joined" orphan_marker)
if(orphan_marker EQUAL -1)
  message(FATAL_ERROR "Unfinished session did not report cancellation and join: ${orphan_err}")
endif()
message(STATUS "PASS m3_3_cancel_unfinished (forced cleanup, exit 1)")
set(artifact "${scripts}/m3_3_cancel_recovery.csv")
if(NOT EXISTS "${artifact}")
  message(FATAL_ERROR "Missing accepted completion artifact after cancellation recovery")
endif()
file(STRINGS "${artifact}" rows)
list(LENGTH rows length)
if(NOT length EQUAL 7)
  message(FATAL_ERROR "Expected exactly 7 stable rows, got ${length}")
endif()
message(STATUS "M3.3 NATIVE SCRIPT CANCEL/RECOVER CTEST PASSED (4 cases, 7 rows)")
