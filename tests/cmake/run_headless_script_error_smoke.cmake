if(NOT DEFINED TEST_EXECUTABLE OR TEST_EXECUTABLE STREQUAL "")
  message(FATAL_ERROR "TEST_EXECUTABLE is required.")
endif()

if(NOT DEFINED OUTPUT_DIR OR OUTPUT_DIR STREQUAL "")
  message(FATAL_ERROR "OUTPUT_DIR is required.")
endif()

file(MAKE_DIRECTORY "${OUTPUT_DIR}")
file(TO_CMAKE_PATH "${OUTPUT_DIR}/run_script_gui_only_api.tnhpps" _script_file)
file(TO_CMAKE_PATH "${OUTPUT_DIR}/run_script_invalid_write_json_path.tnhpps" _invalid_write_json_script)
file(TO_CMAKE_PATH "${OUTPUT_DIR}/run_script_syntax_error.tnhpps" _syntax_error_script)
file(TO_CMAKE_PATH "${OUTPUT_DIR}/run_script_trace_missing_scene.tnhpps" _trace_missing_scene_script)
file(TO_CMAKE_PATH "${OUTPUT_DIR}/run_script_trace_invalid_rays.tnhpps" _trace_invalid_rays_script)
file(TO_CMAKE_PATH "${OUTPUT_DIR}/run_script_trace_invalid_seed.tnhpps" _trace_invalid_seed_script)
file(TO_CMAKE_PATH "${OUTPUT_DIR}/run_script_trace_omitted_no_export.tnhpps" _trace_omitted_no_export_script)
file(TO_CMAKE_PATH "${OUTPUT_DIR}/run_script_trace_false_no_export.tnhpps" _trace_false_no_export_script)
file(TO_CMAKE_PATH "${OUTPUT_DIR}/run_script_trace_invalid_scene.tnhpps" _trace_invalid_scene_script)
file(TO_CMAKE_PATH "${OUTPUT_DIR}/missing_scene.tnhpp" _missing_scene_file)

file(WRITE "${_script_file}" "tn.Screenshot(\"not-supported.png\");
")
file(WRITE "${_invalid_write_json_script}" "tn.writeJson(\"\", { ok: true });
")
file(WRITE "${_syntax_error_script}" "var value = ;
")
file(WRITE "${_trace_missing_scene_script}" "tn.traceScene({ rays: 1, seed: 0, noExport: true });
")
file(WRITE "${_trace_invalid_rays_script}" "tn.traceScene({ scene: \"unused.tnhpp\", rays: 0, seed: 0, noExport: true });
")
file(WRITE "${_trace_invalid_seed_script}" "tn.traceScene({ scene: \"unused.tnhpp\", rays: 1, seed: -1, noExport: true });
")
file(WRITE "${_trace_omitted_no_export_script}" "tn.traceScene({ scene: \"unused.tnhpp\", rays: 1, seed: 0 });
")
file(WRITE "${_trace_false_no_export_script}" "tn.traceScene({ scene: \"unused.tnhpp\", rays: 1, seed: 0, noExport: false });
")
file(WRITE "${_trace_invalid_scene_script}" "tn.traceScene({ scene: \"${_missing_scene_file}\", rays: 1, seed: 0, noExport: true });
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

if(NOT "${_exit_code}" STREQUAL "1")
  message(STATUS "stdout:\n${_stdout}")
  message(STATUS "stderr:\n${_stderr}")
  message(FATAL_ERROR "Expected GUI-only headless script API smoke test to exit 1, got ${_exit_code}.")
endif()

if(NOT _stderr MATCHES "Headless script API does not support 'Screenshot'")
  message(STATUS "stdout:\n${_stdout}")
  message(STATUS "stderr:\n${_stderr}")
  message(FATAL_ERROR "GUI-only headless script API smoke test did not report the unsupported API clearly.")
endif()

execute_process(
  COMMAND "${TEST_EXECUTABLE}" --headless run-script "${_invalid_write_json_script}"
  ${_working_directory}
  RESULT_VARIABLE _exit_code
  OUTPUT_VARIABLE _stdout
  ERROR_VARIABLE _stderr
)

if(NOT "${_exit_code}" STREQUAL "1")
  message(STATUS "stdout:\n${_stdout}")
  message(STATUS "stderr:\n${_stderr}")
  message(FATAL_ERROR "Expected invalid writeJson path smoke test to exit 1, got ${_exit_code}.")
endif()

if(NOT _stderr MATCHES "tn\\.writeJson requires a non-empty path")
  message(STATUS "stdout:\n${_stdout}")
  message(STATUS "stderr:\n${_stderr}")
  message(FATAL_ERROR "Invalid writeJson path smoke test did not report the path error clearly.")
endif()

execute_process(
  COMMAND "${TEST_EXECUTABLE}" --headless run-script "${_syntax_error_script}"
  ${_working_directory}
  RESULT_VARIABLE _exit_code
  OUTPUT_VARIABLE _stdout
  ERROR_VARIABLE _stderr
)

if(NOT "${_exit_code}" STREQUAL "1")
  message(STATUS "stdout:\n${_stdout}")
  message(STATUS "stderr:\n${_stderr}")
  message(FATAL_ERROR "Expected syntax-error headless script smoke test to exit 1, got ${_exit_code}.")
endif()

if(NOT _stderr MATCHES "Script execution failed:.*run_script_syntax_error\\.tnhpps:1")
  message(STATUS "stdout:\n${_stdout}")
  message(STATUS "stderr:\n${_stderr}")
  message(FATAL_ERROR "Syntax-error headless script smoke test did not report file and line clearly.")
endif()

foreach(_case IN ITEMS
    "missing_scene|${_trace_missing_scene_script}|options\\.scene must be a non-empty scene path"
    "invalid_rays|${_trace_invalid_rays_script}|options\\.rays must be greater than zero"
    "invalid_seed|${_trace_invalid_seed_script}|options\\.seed must not be negative"
    "omitted_no_export|${_trace_omitted_no_export_script}|options\\.noExport must be true"
    "false_no_export|${_trace_false_no_export_script}|options\\.noExport must be true"
    "invalid_scene|${_trace_invalid_scene_script}|tn\\.traceScene failed while loading")
  string(REPLACE "|" ";" _parts "${_case}")
  list(GET _parts 0 _name)
  list(GET _parts 1 _script)
  list(GET _parts 2 _pattern)

  execute_process(
    COMMAND "${TEST_EXECUTABLE}" --headless run-script "${_script}"
    ${_working_directory}
    RESULT_VARIABLE _exit_code
    OUTPUT_VARIABLE _stdout
    ERROR_VARIABLE _stderr
  )

  if(NOT "${_exit_code}" STREQUAL "1")
    message(STATUS "stdout:\n${_stdout}")
    message(STATUS "stderr:\n${_stderr}")
    message(FATAL_ERROR "Expected traceScene ${_name} smoke test to exit 1, got ${_exit_code}.")
  endif()

  if(NOT _stderr MATCHES "${_pattern}")
    message(STATUS "stdout:\n${_stdout}")
    message(STATUS "stderr:\n${_stderr}")
    message(FATAL_ERROR "traceScene ${_name} smoke test did not report the expected error.")
  endif()
endforeach()
