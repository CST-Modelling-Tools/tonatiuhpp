# M4.1: direct true-headless CLI versus true-headless scripted File export.
# Two independent CLI processes and one script use the identical real File
# plugin, selected surface, 2,048 rays, 200x200 sun grid and seed zero.
# No frozen benchmark and no assumed ordering of serialized photon records.
cmake_minimum_required(VERSION 3.21)
foreach(required TEST_EXECUTABLE TEST_SCRIPT TEST_SCENE COMPARATOR
                 PYTHON_EXECUTABLE OUTPUT_DIR TEST_BUILD_ROOT TEST_WORKING_DIRECTORY)
  if(NOT DEFINED ${required} OR "${${required}}" STREQUAL "")
    message(FATAL_ERROR "M4 File export parity: missing ${required}")
  endif()
endforeach()
foreach(required TEST_EXECUTABLE TEST_SCRIPT TEST_SCENE COMPARATOR PYTHON_EXECUTABLE)
  if(NOT EXISTS "${${required}}")
    message(FATAL_ERROR "M4 File export parity: missing fixture ${required}: ${${required}}")
  endif()
endforeach()

# Only this one known build-tree fixture directory may be cleared. No wildcard,
# source-tree deletion, Git operation, or user-selected output folder.
get_filename_component(expected "${TEST_BUILD_ROOT}/m4_cli_file_export_parity" ABSOLUTE)
get_filename_component(actual "${OUTPUT_DIR}" ABSOLUTE)
file(TO_CMAKE_PATH "${expected}" expected)
file(TO_CMAKE_PATH "${actual}" actual)
if(NOT actual STREQUAL expected)
  message(FATAL_ERROR "Refusing to clean non-fixture M4 output directory: ${OUTPUT_DIR}")
endif()
file(REMOVE_RECURSE "${OUTPUT_DIR}")
file(MAKE_DIRECTORY "${OUTPUT_DIR}/cli_first" "${OUTPUT_DIR}/cli_second"
                    "${OUTPUT_DIR}/script_export" "${OUTPUT_DIR}/scripts"
                    "${OUTPUT_DIR}/occupied")
file(TO_CMAKE_PATH "${TEST_SCENE}" scene_script_path)
file(TO_CMAKE_PATH "${OUTPUT_DIR}/script_export" script_export_path)
file(READ "${TEST_SCRIPT}" script_template)
string(REPLACE "@SCENE_FILE@" "${scene_script_path}" script_content "${script_template}")
string(REPLACE "@EXPORT_DIRECTORY@" "${script_export_path}" script_content "${script_content}")
set(script_path "${OUTPUT_DIR}/scripts/m4_cli_file_export_parity.tnhpps")
file(WRITE "${script_path}" "${script_content}")

foreach(side cli_first cli_second)
  set(directory "${OUTPUT_DIR}/${side}")
  execute_process(
    COMMAND "${TEST_EXECUTABLE}" --headless trace-scene "${TEST_SCENE}"
      --rays 2048 --seed 0 --photon-export-dir "${directory}"
      --photon-surface "//Node/Shape" --update-trackers
    WORKING_DIRECTORY "${TEST_WORKING_DIRECTORY}"
    RESULT_VARIABLE code OUTPUT_VARIABLE stdout ERROR_VARIABLE stderr
    TIMEOUT 90)
  if(NOT "${code}" STREQUAL "0" OR NOT stdout MATCHES "Trace completed[.]")
    message(FATAL_ERROR "Direct M4 CLI ${side} failed (${code}):\n${stdout}\n${stderr}")
  endif()
  if(NOT EXISTS "${directory}/PhotonMap.dat" OR
      NOT EXISTS "${directory}/PhotonMap_parameters.txt")
    message(FATAL_ERROR "M4 CLI ${side}: missing physical File export")
  endif()
endforeach()

execute_process(
  COMMAND "${TEST_EXECUTABLE}" --headless run-script "${script_path}"
  WORKING_DIRECTORY "${TEST_WORKING_DIRECTORY}"
  RESULT_VARIABLE code OUTPUT_VARIABLE stdout ERROR_VARIABLE stderr TIMEOUT 90)
if(NOT "${code}" STREQUAL "0" OR NOT stdout MATCHES "M4 CLI/script File photon parity fixture completed[.]")
  message(FATAL_ERROR "Scripted M4 File export failed (${code}):\n${stdout}\n${stderr}")
endif()
if(NOT EXISTS "${OUTPUT_DIR}/script_export/PhotonMap.dat" OR
    NOT EXISTS "${OUTPUT_DIR}/script_export/PhotonMap_parameters.txt")
  message(FATAL_ERROR "M4 scripted run: missing physical File export")
endif()

foreach(other cli_second script_export)
  execute_process(
    COMMAND "${PYTHON_EXECUTABLE}" "${COMPARATOR}"
      "${OUTPUT_DIR}/cli_first" "${OUTPUT_DIR}/${other}"
    RESULT_VARIABLE code OUTPUT_VARIABLE stdout ERROR_VARIABLE stderr)
  if(NOT "${code}" STREQUAL "0")
    message(FATAL_ERROR "M4 physical photon parity failed (${other}):\n${stdout}\n${stderr}")
  endif()
  message(STATUS "M4 physical photon parity ${other}: ${stdout}")
endforeach()

# Non-destructive failure: never overwrite previous accepted output.
file(SHA256 "${OUTPUT_DIR}/cli_first/PhotonMap.dat" prior_data)
file(SHA256 "${OUTPUT_DIR}/cli_first/PhotonMap_parameters.txt" prior_meta)
execute_process(
  COMMAND "${TEST_EXECUTABLE}" --headless trace-scene "${TEST_SCENE}"
    --rays 2048 --seed 0 --photon-export-dir "${OUTPUT_DIR}/cli_first"
    --photon-surface "//Node/Shape" --update-trackers
  WORKING_DIRECTORY "${TEST_WORKING_DIRECTORY}"
  RESULT_VARIABLE code OUTPUT_VARIABLE stdout ERROR_VARIABLE stderr TIMEOUT 20)
if("${code}" STREQUAL "0" OR stdout MATCHES "Trace completed[.]")
  message(FATAL_ERROR "M4 CLI overwrote an existing File export")
endif()
file(SHA256 "${OUTPUT_DIR}/cli_first/PhotonMap.dat" after_data)
file(SHA256 "${OUTPUT_DIR}/cli_first/PhotonMap_parameters.txt" after_meta)
if(NOT prior_data STREQUAL after_data OR NOT prior_meta STREQUAL after_meta)
  message(FATAL_ERROR "M4 CLI changed a protected previous photon export")
endif()

# Unrelated files in the output directory must survive untouched.
file(WRITE "${OUTPUT_DIR}/occupied/keep.txt" "M4 fixture must survive\n")
file(SHA256 "${OUTPUT_DIR}/occupied/keep.txt" prior_keep)
execute_process(
  COMMAND "${TEST_EXECUTABLE}" --headless trace-scene "${TEST_SCENE}"
    --rays 2048 --seed 0 --photon-export-dir "${OUTPUT_DIR}/occupied"
    --photon-surface "//Node/Shape" --update-trackers
  WORKING_DIRECTORY "${TEST_WORKING_DIRECTORY}"
  RESULT_VARIABLE code OUTPUT_VARIABLE stdout ERROR_VARIABLE stderr TIMEOUT 20)
file(SHA256 "${OUTPUT_DIR}/occupied/keep.txt" after_keep)
if("${code}" STREQUAL "0" OR NOT prior_keep STREQUAL after_keep OR
   EXISTS "${OUTPUT_DIR}/occupied/PhotonMap.dat")
  message(FATAL_ERROR "M4 CLI failed to preserve unrelated files in occupied folder")
endif()

# A missing output directory must not be created by the File exporter.
set(missing "${OUTPUT_DIR}/missing_do_not_create")
if(EXISTS "${missing}")
  message(FATAL_ERROR "Invalid M4 fixture: missing directory unexpectedly exists")
endif()
execute_process(
  COMMAND "${TEST_EXECUTABLE}" --headless trace-scene "${TEST_SCENE}"
    --rays 2048 --seed 0 --photon-export-dir "${missing}"
    --photon-surface "//Node/Shape" --update-trackers
  WORKING_DIRECTORY "${TEST_WORKING_DIRECTORY}"
  RESULT_VARIABLE code OUTPUT_VARIABLE stdout ERROR_VARIABLE stderr TIMEOUT 20)
if("${code}" STREQUAL "0" OR EXISTS "${missing}" OR stdout MATCHES "Trace completed[.]")
  message(FATAL_ERROR "M4 CLI accepted or created a missing output directory")
endif()
message(STATUS "M4 direct CLI/script real File parity and non-destructive path checks passed")
