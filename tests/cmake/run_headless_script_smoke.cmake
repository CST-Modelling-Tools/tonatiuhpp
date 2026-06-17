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
file(TO_CMAKE_PATH "${OUTPUT_DIR}/run_script_${_run_id}.tnhpps" _script_file)
file(TO_CMAKE_PATH "${OUTPUT_DIR}/run_script_result_${_run_id}.json" _script_output_file)
file(TO_CMAKE_PATH "${OUTPUT_DIR}/benchmark_config_${_run_id}.json" _benchmark_config_file)
file(TO_CMAKE_PATH "${OUTPUT_DIR}/benchmark_result_${_run_id}.json" _benchmark_output_file)

file(WRITE "${_benchmark_config_file}" "{
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
  \"output_file\": \"${_benchmark_output_file}\"
}
")

file(WRITE "${_script_file}" "print(\"headless run-script smoke\");

if (!tn.validateScene(\"${_scene_file}\")) {
  throw new Error(\"tn.validateScene returned false\");
}

var trace = tn.traceScene({
  scene: \"${_scene_file}\",
  rays: 10,
  seed: 123456789,
  noExport: true
});
if (!trace || trace.rays_traced !== 10 || trace.photon_export !== false) {
  throw new Error(\"tn.traceScene did not return the expected no-export summary\");
}
print(\"trace rays \" + trace.rays_traced);

tn.writeJson(\"${_script_output_file}\", {
  ok: true,
  scene_file: \"${_scene_file}\",
  trace: trace,
  benchmark_config: \"${_benchmark_config_file}\"
});

var benchmarkExitCode = tn.runBenchmark(\"${_benchmark_config_file}\");
if (benchmarkExitCode !== 0) {
  throw new Error(\"tn.runBenchmark returned \" + benchmarkExitCode);
}
")

set(_working_directory)
if(DEFINED TEST_WORKING_DIRECTORY AND NOT TEST_WORKING_DIRECTORY STREQUAL "")
  set(_working_directory WORKING_DIRECTORY "${TEST_WORKING_DIRECTORY}")
endif()

execute_process(
  COMMAND "${TEST_EXECUTABLE}" --headless run-script "${_script_file}"
  ${_working_directory}
  RESULT_VARIABLE _exit_code
  OUTPUT_VARIABLE _stdout
  ERROR_VARIABLE _stderr
)

if(NOT "${_exit_code}" STREQUAL "0")
  message(STATUS "stdout:\n${_stdout}")
  message(STATUS "stderr:\n${_stderr}")
  message(FATAL_ERROR "Expected headless run-script smoke test to exit 0, got ${_exit_code}.")
endif()

foreach(_pattern
    "headless run-script smoke"
    "trace rays 10"
    "Benchmark completed\\."
    "scene_file:"
    "rays:"
    "seed:"
    "result_file:")
  if(NOT _stdout MATCHES "${_pattern}")
    message(STATUS "stdout:\n${_stdout}")
    message(STATUS "stderr:\n${_stderr}")
    message(FATAL_ERROR "Headless run-script smoke test output did not match: ${_pattern}")
  endif()
endforeach()

if(NOT EXISTS "${_script_output_file}")
  message(STATUS "stdout:\n${_stdout}")
  message(STATUS "stderr:\n${_stderr}")
  message(FATAL_ERROR "Headless run-script smoke test did not write script JSON: ${_script_output_file}")
endif()

if(NOT EXISTS "${_benchmark_output_file}")
  message(STATUS "stdout:\n${_stdout}")
  message(STATUS "stderr:\n${_stderr}")
  message(FATAL_ERROR "Headless run-script smoke test did not write benchmark JSON: ${_benchmark_output_file}")
endif()

file(READ "${_script_output_file}" _script_json)
foreach(_key
    "ok"
    "scene_file"
    "trace"
    "benchmark_config")
  if(NOT _script_json MATCHES "\"${_key}\"")
    message(STATUS "script JSON:\n${_script_json}")
    message(FATAL_ERROR "Headless run-script JSON did not contain key: ${_key}")
  endif()
endforeach()
