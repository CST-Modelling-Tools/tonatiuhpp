foreach(_key IN ITEMS TEST_EXECUTABLE TEST_WORKING_DIRECTORY SCENE_FILE OUTPUT_DIR PYTHON_EXECUTABLE SOURCE_DIR)
  if(NOT DEFINED ${_key} OR "${${_key}}" STREQUAL "")
    message(FATAL_ERROR "${_key} is required")
  endif()
endforeach()
# Optional test-only GUI-lifecycle tracker preparation. Omitted for the
# established cylinder regression, preserving its original CLI behavior.
set(_tracker_args)
if(UPDATE_TRACKERS)
  list(APPEND _tracker_args --update-trackers)
endif()

file(MAKE_DIRECTORY "${OUTPUT_DIR}")
foreach(_run IN ITEMS first second baseline)
  set(_signature "${OUTPUT_DIR}/${_run}_signature.json")
  file(REMOVE "${_signature}")
  if(NOT _run STREQUAL "baseline")
    set(_output "${OUTPUT_DIR}/${_run}")
    file(REMOVE_RECURSE "${_output}")
    file(MAKE_DIRECTORY "${_output}")
    set(_export_args --photon-export-dir "${_output}")
  else()
    set(_export_args --no-export)
  endif()
  execute_process(
    COMMAND "${CMAKE_COMMAND}" -E env
      "TONATIUHPP_A0_TRACE_SIGNATURE_FILE=${_signature}"
      "${TEST_EXECUTABLE}" --headless trace-scene "${SCENE_FILE}"
      --rays 20001 --seed 123456789 ${_export_args} ${_tracker_args}
    WORKING_DIRECTORY "${TEST_WORKING_DIRECTORY}"
    RESULT_VARIABLE _exit OUTPUT_VARIABLE _stdout ERROR_VARIABLE _stderr
    TIMEOUT 120
  )
  string(FIND "${_stdout}" "Trace completed." _completed)
  string(FIND "${_stdout}" "tracker_update: enabled" _tracker_updated)
  if(NOT "${_exit}" STREQUAL "0" OR _completed EQUAL -1
      OR NOT EXISTS "${_signature}"
      OR (UPDATE_TRACKERS AND _tracker_updated EQUAL -1)
      OR (NOT UPDATE_TRACKERS AND NOT _tracker_updated EQUAL -1))
    message(STATUS "${_run} stdout: ${_stdout}")
    message(STATUS "${_run} stderr: ${_stderr}")
    message(FATAL_ERROR "${_run}: photon trace process failed")
  endif()
endforeach()
execute_process(
  COMMAND "${PYTHON_EXECUTABLE}" "${SOURCE_DIR}/scientific/compare_native_trace_signatures.py"
    "${OUTPUT_DIR}/first_signature.json" "${OUTPUT_DIR}/second_signature.json"
  RESULT_VARIABLE _match_a OUTPUT_VARIABLE _out_a ERROR_VARIABLE _err_a)
execute_process(
  COMMAND "${PYTHON_EXECUTABLE}" "${SOURCE_DIR}/scientific/compare_native_trace_signatures.py"
    "${OUTPUT_DIR}/first_signature.json" "${OUTPUT_DIR}/baseline_signature.json"
  RESULT_VARIABLE _match_b OUTPUT_VARIABLE _out_b ERROR_VARIABLE _err_b)
execute_process(
  COMMAND "${PYTHON_EXECUTABLE}" "${SOURCE_DIR}/scientific/compare_photon_exports.py"
    "${OUTPUT_DIR}/first" "${OUTPUT_DIR}/second"
  RESULT_VARIABLE _match_c OUTPUT_VARIABLE _out_c ERROR_VARIABLE _err_c)
if(NOT _match_a EQUAL 0 OR NOT _match_b EQUAL 0 OR NOT _match_c EQUAL 0)
  message(FATAL_ERROR "Photon export scientific mismatch:\n${_out_a}${_err_a}\n${_out_b}${_err_b}\n${_out_c}${_err_c}")
endif()
message(STATUS "${_out_a}${_out_b}${_out_c}")
