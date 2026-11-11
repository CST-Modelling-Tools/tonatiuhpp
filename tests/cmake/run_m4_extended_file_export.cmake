# M4 completion candidate: segmented File output, cumulative append, 210001-ray
# flush/parity, basename refusal and no-export option refusal, no frozen 500M.
cmake_minimum_required(VERSION 3.21)
foreach(required TEST_EXECUTABLE TEST_SCRIPT TEST_SCENE COMPARATOR
                 PYTHON_EXECUTABLE OUTPUT_DIR TEST_BUILD_ROOT TEST_WORKING_DIRECTORY)
  if(NOT DEFINED ${required} OR "${${required}}" STREQUAL "")
    message(FATAL_ERROR "M4 extended File: missing ${required}")
  endif()
endforeach()
foreach(required TEST_EXECUTABLE TEST_SCRIPT TEST_SCENE COMPARATOR PYTHON_EXECUTABLE)
  if(NOT EXISTS "${${required}}")
    message(FATAL_ERROR "M4 extended File: fixture missing ${required}")
  endif()
endforeach()
get_filename_component(expected "${TEST_BUILD_ROOT}/m4_extended_file_export" ABSOLUTE)
get_filename_component(actual "${OUTPUT_DIR}" ABSOLUTE)
file(TO_CMAKE_PATH "${expected}" expected)
file(TO_CMAKE_PATH "${actual}" actual)
if(NOT actual STREQUAL expected OR IS_SYMLINK "${OUTPUT_DIR}")
  message(FATAL_ERROR "Refusing unsafe test cleanup: ${OUTPUT_DIR}")
endif()
file(REMOVE_RECURSE "${OUTPUT_DIR}")
file(MAKE_DIRECTORY "${OUTPUT_DIR}" "${OUTPUT_DIR}/scripts")
file(TO_CMAKE_PATH "${TEST_SCENE}" scene_path)
file(READ "${TEST_SCRIPT}" script_template)

function(m4_script suffix ray_count file_size append repeat)
  set(target_dir "${OUTPUT_DIR}/${suffix}")
  file(MAKE_DIRECTORY "${target_dir}")
  file(TO_CMAKE_PATH "${target_dir}" export_path)
  set(source "${script_template}")
  string(REPLACE "@SCENE_FILE@" "${scene_path}" source "${source}")
  string(REPLACE "@EXPORT_DIRECTORY@" "${export_path}" source "${source}")
  string(REPLACE "@RAYS@" "${ray_count}" source "${source}")
  string(REPLACE "@FILE_SIZE@" "${file_size}" source "${source}")
  string(REPLACE "@APPEND@" "${append}" source "${source}")
  string(REPLACE "@RUN_AGAIN@" "${repeat}" source "${source}")
  set(out "${OUTPUT_DIR}/scripts/${suffix}.tnhpps")
  file(WRITE "${out}" "${source}")
  execute_process(
    COMMAND "${TEST_EXECUTABLE}"  run-script "${out}"
    WORKING_DIRECTORY "${TEST_WORKING_DIRECTORY}"
    RESULT_VARIABLE rc OUTPUT_VARIABLE stdout ERROR_VARIABLE stderr TIMEOUT 140)
  if(NOT "${rc}" STREQUAL "0" OR NOT stdout MATCHES "M4 extended File export fixture completed[.]")
    message(FATAL_ERROR "M4 ${suffix} failed: ${rc}\n${stdout}\n${stderr}")
  endif()
endfunction()

function(m4_cli suffix ray_count file_size)
  set(target_dir "${OUTPUT_DIR}/${suffix}")
  file(MAKE_DIRECTORY "${target_dir}")
  execute_process(
    COMMAND "${TEST_EXECUTABLE}"  trace-scene "${TEST_SCENE}"
      --rays "${ray_count}" --seed 0 --photon-export-dir "${target_dir}"
      --photon-surface "//Node/Shape" --update-trackers
      --photon-file-name "PhotonMap" --photon-file-size "${file_size}"
    WORKING_DIRECTORY "${TEST_WORKING_DIRECTORY}"
    RESULT_VARIABLE rc OUTPUT_VARIABLE stdout ERROR_VARIABLE stderr TIMEOUT 140)
  if(NOT "${rc}" STREQUAL "0" OR NOT stdout MATCHES "Trace completed[.]")
    message(FATAL_ERROR "M4 CLI ${suffix} failed: ${rc}\n${stdout}\n${stderr}")
  endif()
endfunction()

function(m4_compare first second)
  execute_process(
    COMMAND "${PYTHON_EXECUTABLE}" "${COMPARATOR}"
       "${OUTPUT_DIR}/${first}" "${OUTPUT_DIR}/${second}" ${ARGN}
    RESULT_VARIABLE rc OUTPUT_VARIABLE stdout ERROR_VARIABLE stderr TIMEOUT 80)
  if(NOT "${rc}" STREQUAL "0")
    message(FATAL_ERROR "M4 physical equality ${first}/${second} failed: ${rc}\n${stdout}\n${stderr}")
  endif()
  message(STATUS "${stdout}")
endfunction()

# Both direct CLI and shared-script host must produce the same native 128-
# photon-per-file segments (the same physical records and normalizing power).
m4_cli(cli_segments 4096 128)
m4_script(script_segments 4096 128 false "")
m4_compare(cli_segments script_segments --expect-segments --max-per-file 128)

# A second tn.Run(0) from the same persistent script host must append its
# same-seed records rather than overwrite the original segments; normalized
# per-ray photon power should halve for twice the cumulative rays.
m4_script(script_append 4096 128 true "tn.Run(0);")
m4_compare(script_segments script_append --expect-duplicate --expect-segments --max-per-file 128)

# Both adapters must move beyond the former 200000 cap, flush multiple buffers,
# and remain physically equivalent with one-file output (210001 rays).
m4_cli(cli_large 210001 -1)
m4_script(script_large 210001 -1 false "")
m4_compare(cli_large script_large)

# Invalid basename must fail without creating any export file or writing into
# the parent. We intentionally pass a relative parent traversal string.
file(MAKE_DIRECTORY "${OUTPUT_DIR}/invalid_basename")
execute_process(
  COMMAND "${TEST_EXECUTABLE}"  trace-scene "${TEST_SCENE}"
    --rays 2048 --seed 0 --photon-export-dir "${OUTPUT_DIR}/invalid_basename"
    --photon-file-name "../unsafe" --photon-file-size 128
  WORKING_DIRECTORY "${TEST_WORKING_DIRECTORY}"
  RESULT_VARIABLE rc OUTPUT_VARIABLE stdout ERROR_VARIABLE stderr TIMEOUT 30)
if("${rc}" STREQUAL "0" OR EXISTS "${OUTPUT_DIR}/unsafe.dat" OR
   EXISTS "${OUTPUT_DIR}/invalid_basename/unsafe_1.dat")
  message(FATAL_ERROR "M4 unsafe basename was accepted or wrote outside output: ${stdout}\n${stderr}")
endif()

execute_process(
  COMMAND "${TEST_EXECUTABLE}"  trace-scene "${TEST_SCENE}"
    --rays 2048 --seed 0 --no-export --photon-file-size 128
  WORKING_DIRECTORY "${TEST_WORKING_DIRECTORY}"
  RESULT_VARIABLE rc OUTPUT_VARIABLE stdout ERROR_VARIABLE stderr TIMEOUT 30)
if("${rc}" STREQUAL "0")
  message(FATAL_ERROR "M4 accepted FileSize without File exporter")
endif()

message(STATUS "M4 extended File/CLI/script/append/flush/unsafe-path acceptance passed")
