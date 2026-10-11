foreach(_required IN ITEMS TEST_EXECUTABLE TEST_WORKING_DIRECTORY SCENE_FILE OUTPUT_DIR PYTHON_EXECUTABLE SOURCE_DIR)
  if(NOT DEFINED ${_required} OR "${${_required}}" STREQUAL "")
    message(FATAL_ERROR "Missing ${_required}")
  endif()
endforeach()
file(MAKE_DIRECTORY "${OUTPUT_DIR}")
set(_tracker_args)
if(UPDATE_TRACKERS)
  list(APPEND _tracker_args --update-trackers)
endif()
foreach(_mode IN ITEMS record_one record_two no_record)
  set(_history "${OUTPUT_DIR}/${_mode}_history.json")
  set(_hits "${OUTPUT_DIR}/${_mode}_hits.json")
  file(REMOVE "${_history}" "${_hits}")
  set(_export_args --no-export)
  if(NOT _mode STREQUAL "no_record")
    set(_export_dir "${OUTPUT_DIR}/${_mode}_photons")
    file(REMOVE_RECURSE "${_export_dir}")
    file(MAKE_DIRECTORY "${_export_dir}")
    set(_export_args --photon-export-dir "${_export_dir}")
  endif()
  execute_process(
    COMMAND "${CMAKE_COMMAND}" -E env
      "TONATIUHPP_A0_RAY_HISTORY_FILE=${_history}"
      "TONATIUHPP_A0_TRACE_SIGNATURE_FILE=${_hits}"
      "${TEST_EXECUTABLE}" --headless trace-scene "${SCENE_FILE}"
      --rays 20001 --seed 123456789 ${_export_args} ${_tracker_args}
    WORKING_DIRECTORY "${TEST_WORKING_DIRECTORY}"
    RESULT_VARIABLE _code
    OUTPUT_VARIABLE _stdout ERROR_VARIABLE _stderr TIMEOUT 120)
  string(FIND "${_stdout}" "Trace completed." _complete)
  string(FIND "${_stdout}" "a0_headless_ray_history:" _history_print)
  string(FIND "${_stdout}" "tracker_update: enabled" _tracked)
  if(NOT "${_code}" STREQUAL "0" OR _complete EQUAL -1 OR _history_print EQUAL -1
      OR NOT EXISTS "${_history}" OR NOT EXISTS "${_hits}"
      OR (UPDATE_TRACKERS AND _tracked EQUAL -1))
    message(STATUS "${_mode} stdout: ${_stdout}")
    message(STATUS "${_mode} stderr: ${_stderr}")
    message(FATAL_ERROR "Native ordered ray history trace failed: ${_mode}")
  endif()
endforeach()
foreach(_other IN ITEMS record_two no_record)
  execute_process(
    COMMAND "${PYTHON_EXECUTABLE}" "${SOURCE_DIR}/scientific/compare_native_ray_histories.py"
      "${OUTPUT_DIR}/record_one_history.json" "${OUTPUT_DIR}/${_other}_history.json"
    RESULT_VARIABLE _code OUTPUT_VARIABLE _stdout ERROR_VARIABLE _stderr)
  if(NOT _code EQUAL 0)
    message(FATAL_ERROR "Ray histories mismatch: ${_other}\n${_stdout}${_stderr}")
  endif()
  message(STATUS "${_stdout}")
  execute_process(
    COMMAND "${PYTHON_EXECUTABLE}" "${SOURCE_DIR}/scientific/compare_native_trace_signatures.py"
      "${OUTPUT_DIR}/record_one_hits.json" "${OUTPUT_DIR}/${_other}_hits.json"
    RESULT_VARIABLE _hitcode OUTPUT_VARIABLE _hitstdout ERROR_VARIABLE _hitstderr)
  if(NOT _hitcode EQUAL 0)
    message(FATAL_ERROR "Exact-hit science mismatch: ${_other}\n${_hitstdout}${_hitstderr}")
  endif()
endforeach()
execute_process(
  COMMAND "${PYTHON_EXECUTABLE}" "${SOURCE_DIR}/scientific/compare_photon_exports.py"
    "${OUTPUT_DIR}/record_one_photons" "${OUTPUT_DIR}/record_two_photons"
  RESULT_VARIABLE _photoncode OUTPUT_VARIABLE _photonstdout ERROR_VARIABLE _photonstderr)
if(NOT _photoncode EQUAL 0)
  message(FATAL_ERROR "Canonical photon files differ\n${_photonstdout}${_photonstderr}")
endif()
message(STATUS "${_photonstdout}")
