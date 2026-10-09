# Repeatability is tested in two independently launched processes.
# Only scientific results are compared; timings and worker counts can vary.
foreach(_name IN ITEMS TEST_EXECUTABLE SCENE_FILE OUTPUT_DIR)
  if(NOT DEFINED ${_name} OR "${${_name}}" STREQUAL "")
    message(FATAL_ERROR "${_name} is required.")
  endif()
endforeach()

file(MAKE_DIRECTORY "${OUTPUT_DIR}")
string(RANDOM LENGTH 8 ALPHABET "0123456789abcdef" _run_id)
file(TO_CMAKE_PATH "${SCENE_FILE}" _scene_file)

set(_working_directory)
if(DEFINED TEST_WORKING_DIRECTORY AND NOT TEST_WORKING_DIRECTORY STREQUAL "")
  set(_working_directory WORKING_DIRECTORY "${TEST_WORKING_DIRECTORY}")
endif()

# A 20,001-ray trace spans three existing deterministic 10,000-ray chunks.
foreach(_run IN ITEMS 1 2)
  file(TO_CMAKE_PATH "${OUTPUT_DIR}/result_${_run_id}_${_run}.json" _result_file)
  file(TO_CMAKE_PATH "${OUTPUT_DIR}/config_${_run_id}_${_run}.json" _config_file)
  file(WRITE "${_config_file}" "{
  \"benchmark\": \"benchmark_v1\",
  \"scene_file\": \"${_scene_file}\",
  \"rays\": 20001,
  \"seed\": 123456789,
  \"target_side_id\": 1,
  \"target_bounds\": {
    \"x_min\": -2.0,
    \"x_max\": 2.0,
    \"y_min\": -2.0,
    \"y_max\": 2.0
  },
  \"target_grid\": {
    \"width\": 8,
    \"height\": 8
  },
  \"photon_export\": false,
  \"output_file\": \"${_result_file}\"
}
")

  execute_process(
    COMMAND "${TEST_EXECUTABLE}" --headless benchmark "${_config_file}"
    ${_working_directory}
    RESULT_VARIABLE _exit_code
    OUTPUT_VARIABLE _stdout
    ERROR_VARIABLE _stderr
    TIMEOUT 90
  )
  string(FIND "${_stdout}" "Benchmark completed." _complete_at)
  if(NOT "${_exit_code}" STREQUAL "0" OR _complete_at EQUAL -1)
    message(STATUS "stdout:\n${_stdout}")
    message(STATUS "stderr:\n${_stderr}")
    message(FATAL_ERROR "Benchmark process ${_run} failed: exit=${_exit_code}")
  endif()
  if(NOT EXISTS "${_result_file}")
    message(FATAL_ERROR "Benchmark process ${_run} did not create: ${_result_file}")
  endif()
  file(READ "${_result_file}" _result)
  foreach(_field IN ITEMS rays seed total_power_mw minimum_flux_mw_m2
      average_flux_mw_m2 maximum_flux_mw_m2 flux_grid_sha256)
    string(JSON _value ERROR_VARIABLE _json_error GET "${_result}" "${_field}")
    if(NOT "${_json_error}" STREQUAL "NOTFOUND")
      message(FATAL_ERROR "Benchmark process ${_run}: missing or invalid ${_field}: ${_json_error}")
    endif()
    if(_run EQUAL 1)
      set("_reference_${_field}" "${_value}")
    elseif(NOT "${_value}" STREQUAL "${_reference_${_field}}")
      message(FATAL_ERROR "Cross-process mismatch in ${_field}: first='${_reference_${_field}}', second='${_value}'")
    endif()
  endforeach()
endforeach()

if(NOT "${_reference_rays}" STREQUAL "20001" OR NOT "${_reference_seed}" STREQUAL "123456789")
  message(FATAL_ERROR "Unexpected benchmark ray count or seed.")
endif()
string(LENGTH "${_reference_flux_grid_sha256}" _hash_length)
if(NOT _hash_length EQUAL 64 OR NOT _reference_flux_grid_sha256 MATCHES "^[0-9a-fA-F]+$")
  message(FATAL_ERROR "Invalid flux grid SHA-256: ${_reference_flux_grid_sha256}")
endif()
message(STATUS "Cross-process scientific repeatability verified: 20,001 rays; SHA-256=${_reference_flux_grid_sha256}")
