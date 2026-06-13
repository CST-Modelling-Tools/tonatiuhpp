if(NOT DEFINED TEST_EXECUTABLE OR TEST_EXECUTABLE STREQUAL "")
  message(FATAL_ERROR "TEST_EXECUTABLE is required.")
endif()

if(NOT DEFINED SCENE_FILE OR SCENE_FILE STREQUAL "")
  message(FATAL_ERROR "SCENE_FILE is required.")
endif()

if(NOT DEFINED OUTPUT_DIR OR OUTPUT_DIR STREQUAL "")
  message(FATAL_ERROR "OUTPUT_DIR is required.")
endif()

file(MAKE_DIRECTORY "${OUTPUT_DIR}")
string(RANDOM LENGTH 8 ALPHABET "0123456789abcdef" _run_id)

file(TO_CMAKE_PATH "${SCENE_FILE}" _scene_file)
file(TO_CMAKE_PATH "${OUTPUT_DIR}/benchmark_result_${_run_id}.json" _output_file)
file(TO_CMAKE_PATH "${OUTPUT_DIR}/benchmark_config.json" _config_file)

file(WRITE "${_config_file}" "{
  \"benchmark\": \"benchmark_v1\",
  \"scene_file\": \"${_scene_file}\",
  \"rays\": 10,
  \"seed\": 123456789,
  \"worker_count\": 1,
  \"chunk_size\": 10,
  \"target_side_id\": 1,
  \"target_bounds\": {
    \"x_min\": -2.0,
    \"x_max\": 2.0,
    \"y_min\": -2.0,
    \"y_max\": 2.0
  },
  \"target_grid\": {
    \"width\": 4,
    \"height\": 4
  },
  \"photon_export\": false,
  \"output_file\": \"${_output_file}\"
}
")

set(_working_directory)
if(DEFINED TEST_WORKING_DIRECTORY AND NOT TEST_WORKING_DIRECTORY STREQUAL "")
  set(_working_directory WORKING_DIRECTORY "${TEST_WORKING_DIRECTORY}")
endif()

execute_process(
  COMMAND "${TEST_EXECUTABLE}" --headless benchmark "${_config_file}"
  ${_working_directory}
  RESULT_VARIABLE _exit_code
  OUTPUT_VARIABLE _stdout
  ERROR_VARIABLE _stderr
)

if(NOT "${_exit_code}" STREQUAL "0")
  message(STATUS "stdout:\n${_stdout}")
  message(STATUS "stderr:\n${_stderr}")
  message(FATAL_ERROR "Expected benchmark smoke test to exit 0, got ${_exit_code}.")
endif()

foreach(_pattern
    "Benchmark completed\\."
    "scene_file:"
    "rays:"
    "seed:"
    "photon_export:"
    "elapsed_seconds:"
    "rays_per_second:"
    "worker_count:"
    "chunk_count:"
    "chunk_size:"
    "result_file:")
  if(NOT _stdout MATCHES "${_pattern}")
    message(STATUS "stdout:\n${_stdout}")
    message(STATUS "stderr:\n${_stderr}")
    message(FATAL_ERROR "Benchmark smoke test output did not match: ${_pattern}")
  endif()
endforeach()

if(NOT EXISTS "${_output_file}")
  message(STATUS "stdout:\n${_stdout}")
  message(STATUS "stderr:\n${_stderr}")
  message(FATAL_ERROR "Benchmark smoke test did not write result JSON: ${_output_file}")
endif()

file(READ "${_output_file}" _result_json)
foreach(_key
    "scene_file"
    "rays"
    "seed"
    "elapsed_seconds"
    "rays_per_second"
    "worker_count"
    "chunk_count"
    "chunk_size")
  if(NOT _result_json MATCHES "\"${_key}\"")
    message(STATUS "result JSON:\n${_result_json}")
    message(FATAL_ERROR "Benchmark result JSON did not contain key: ${_key}")
  endif()
endforeach()
