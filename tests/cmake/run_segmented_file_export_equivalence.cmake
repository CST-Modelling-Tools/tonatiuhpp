# File export completion candidate: segmented File output, cumulative append, 210001-ray
# flush/parity, basename refusal and no-export option refusal, no frozen 500M.
cmake_minimum_required(VERSION 3.21)
foreach(required TEST_EXECUTABLE TEST_SCRIPT TEST_SCENE COMPARATOR
                 PYTHON_EXECUTABLE OUTPUT_DIR TEST_BUILD_ROOT TEST_WORKING_DIRECTORY)
  if(NOT DEFINED ${required} OR "${${required}}" STREQUAL "")
    message(FATAL_ERROR "File export extended File: missing ${required}")
  endif()
endforeach()
foreach(required TEST_EXECUTABLE TEST_SCRIPT TEST_SCENE COMPARATOR PYTHON_EXECUTABLE)
  if(NOT EXISTS "${${required}}")
    message(FATAL_ERROR "File export extended File: fixture missing ${required}")
  endif()
endforeach()
get_filename_component(expected "${TEST_BUILD_ROOT}/segmented_file_export_equivalence" ABSOLUTE)
get_filename_component(actual "${OUTPUT_DIR}" ABSOLUTE)
file(TO_CMAKE_PATH "${expected}" expected)
file(TO_CMAKE_PATH "${actual}" actual)
if(NOT actual STREQUAL expected OR IS_SYMLINK "${OUTPUT_DIR}")
  message(FATAL_ERROR "Refusing unsafe test cleanup: ${OUTPUT_DIR}")
endif()
include("${CMAKE_CURRENT_LIST_DIR}/PrepareTestOutput.cmake")
tonatiuhpp_prepare_test_output("${OUTPUT_DIR}" "${TEST_BUILD_ROOT}" "segmented_file_export_equivalence")
file(MAKE_DIRECTORY "${OUTPUT_DIR}" "${OUTPUT_DIR}/scripts")
file(TO_CMAKE_PATH "${TEST_SCENE}" scene_path)
file(READ "${TEST_SCRIPT}" script_template)

function(scripted_file_export suffix ray_count file_size append repeat)
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
  if(NOT "${rc}" STREQUAL "0" OR NOT stdout MATCHES "Segmented File export fixture completed[.]")
    message(FATAL_ERROR "File export ${suffix} failed: ${rc}\n${stdout}\n${stderr}")
  endif()
endfunction()

function(cli_file_export suffix ray_count file_size)
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
    message(FATAL_ERROR "File export CLI ${suffix} failed: ${rc}\n${stdout}\n${stderr}")
  endif()
endfunction()

function(compare_file_exports first second)
  execute_process(
    COMMAND "${PYTHON_EXECUTABLE}" "${COMPARATOR}"
       "${OUTPUT_DIR}/${first}" "${OUTPUT_DIR}/${second}" ${ARGN}
    RESULT_VARIABLE rc OUTPUT_VARIABLE stdout ERROR_VARIABLE stderr TIMEOUT 80)
  if(NOT "${rc}" STREQUAL "0")
    message(FATAL_ERROR "File export physical equality ${first}/${second} failed: ${rc}\n${stdout}\n${stderr}")
  endif()
  message(STATUS "${stdout}")
endfunction()

# Both direct CLI and shared-script host must produce the same native 128-
# photon-per-file segments (the same physical records and normalizing power).
cli_file_export(cli_segments 4096 128)
scripted_file_export(script_segments 4096 128 false "")
compare_file_exports(cli_segments script_segments --expect-segments --max-per-file 128)

# A second tn.Run(0) from the same persistent script host must append its
# same-seed records rather than overwrite the original segments; normalized
# per-ray photon power should halve for twice the cumulative rays.
scripted_file_export(script_append 4096 128 true "tn.Run(0);")
compare_file_exports(script_segments script_append --expect-duplicate --expect-segments --max-per-file 128)

# Both adapters must move beyond the former 200000 cap, flush multiple buffers,
# and remain physically equivalent with one-file output (210001 rays).
cli_file_export(cli_large 210001 -1)
scripted_file_export(script_large 210001 -1 false "")
compare_file_exports(cli_large script_large)

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
  message(FATAL_ERROR "File export unsafe basename was accepted or wrote outside output: ${stdout}\n${stderr}")
endif()

execute_process(
  COMMAND "${TEST_EXECUTABLE}"  trace-scene "${TEST_SCENE}"
    --rays 2048 --seed 0 --no-export --photon-file-size 128
  WORKING_DIRECTORY "${TEST_WORKING_DIRECTORY}"
  RESULT_VARIABLE rc OUTPUT_VARIABLE stdout ERROR_VARIABLE stderr TIMEOUT 30)
if("${rc}" STREQUAL "0")
  message(FATAL_ERROR "File export accepted FileSize without File exporter")
endif()

message(STATUS "File export extended File/CLI/script/append/flush/unsafe-path acceptance passed")
