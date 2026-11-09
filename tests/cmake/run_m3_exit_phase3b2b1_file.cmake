# Phase 3B2B1: bounded real production File-export plugin smoke, *not* 500M.
cmake_minimum_required(VERSION 3.21)
foreach(v TEST_EXECUTABLE TEST_SCRIPT TEST_SCENE COMPARATOR)
  if(NOT DEFINED ${v} OR "${${v}}" STREQUAL "" OR NOT EXISTS "${${v}}")
    message(FATAL_ERROR "Phase 3B2B1 missing ${v}: ${${v}}")
  endif()
endforeach()
if(NOT DEFINED OUTPUT_DIR OR OUTPUT_DIR STREQUAL "")
  message(FATAL_ERROR "Phase 3B2B1 missing OUTPUT_DIR")
endif()
# Never remove arbitrary filesystem trees from a test entry point.
if(NOT DEFINED TEST_BUILD_ROOT OR TEST_BUILD_ROOT STREQUAL "")
  message(FATAL_ERROR "Phase 3B2B1 requires its registered build/test root")
endif()
get_filename_component(expected_output
  "${TEST_BUILD_ROOT}/m3_exit_phase3b2b1_run_file" ABSOLUTE)
get_filename_component(actual_output "${OUTPUT_DIR}" ABSOLUTE)
file(TO_CMAKE_PATH "${expected_output}" expected_output)
file(TO_CMAKE_PATH "${actual_output}" actual_output)
if(NOT "${actual_output}" STREQUAL "${expected_output}")
  message(FATAL_ERROR "Phase 3B2B1 refuses non-fixture output path: ${OUTPUT_DIR}")
endif()
find_program(PHASE3B2B1_PYTHON NAMES python3 python REQUIRED)
file(REMOVE_RECURSE "${OUTPUT_DIR}/headless_a" "${OUTPUT_DIR}/headless_b" "${OUTPUT_DIR}/gui")
file(MAKE_DIRECTORY "${OUTPUT_DIR}/headless_a" "${OUTPUT_DIR}/headless_b"
                    "${OUTPUT_DIR}/gui" "${OUTPUT_DIR}/script")
file(TO_CMAKE_PATH "${TEST_SCENE}" scene_path)
file(READ "${TEST_SCRIPT}" template)
foreach(side headless_a headless_b gui)
  file(TO_CMAKE_PATH "${OUTPUT_DIR}/${side}" export_path)
  string(REPLACE "@SCENE_FILE@" "${scene_path}" content "${template}")
  string(REPLACE "@EXPORT_DIRECTORY@" "${export_path}" content "${content}")
  file(WRITE "${OUTPUT_DIR}/script/m3_exit_phase3b2b1_${side}.tnhpps" "${content}")
endforeach()

foreach(side headless_a headless_b)
  execute_process(
    COMMAND "${TEST_EXECUTABLE}" --headless run-script
      "${OUTPUT_DIR}/script/m3_exit_phase3b2b1_${side}.tnhpps"
    WORKING_DIRECTORY "${OUTPUT_DIR}/script"
    RESULT_VARIABLE code OUTPUT_VARIABLE stdout ERROR_VARIABLE stderr
    TIMEOUT 90)
  if(NOT "${code}" STREQUAL "0" OR
     NOT stdout MATCHES "M3 Phase 3B2B1 bounded File Run passed")
    message(FATAL_ERROR "Phase 3B2B1 ${side} File run failed (${code}):\n${stdout}\n${stderr}")
  endif()
  if(NOT EXISTS "${OUTPUT_DIR}/${side}/PhotonMap.dat" OR
     NOT EXISTS "${OUTPUT_DIR}/${side}/PhotonMap_parameters.txt")
    message(FATAL_ERROR "Phase 3B2B1 ${side} did not write both production photon files")
  endif()
  file(SIZE "${OUTPUT_DIR}/${side}/PhotonMap.dat" bytes)
  if(bytes LESS 48)
    message(FATAL_ERROR "Phase 3B2B1 ${side}: empty or truncated photon file")
  endif()
endforeach()

execute_process(
  COMMAND "${PHASE3B2B1_PYTHON}" "${COMPARATOR}"
    "${OUTPUT_DIR}/headless_a" "${OUTPUT_DIR}/headless_b"
  RESULT_VARIABLE parity_code OUTPUT_VARIABLE parity_out ERROR_VARIABLE parity_err)
if(NOT "${parity_code}" STREQUAL "0")
  message(FATAL_ERROR "Phase 3B2B1 real File record repeatability mismatch:\n${parity_out}\n${parity_err}")
endif()

# The File plugin may historically overwrite a prior map. This *new*
# headless scripted Run adapter must instead refuse any nonempty directory.
file(SHA256 "${OUTPUT_DIR}/headless_a/PhotonMap.dat" initial_hash)
execute_process(
  COMMAND "${TEST_EXECUTABLE}" --headless run-script
    "${OUTPUT_DIR}/script/m3_exit_phase3b2b1_headless_a.tnhpps"
  WORKING_DIRECTORY "${OUTPUT_DIR}/script"
  RESULT_VARIABLE rejected OUTPUT_VARIABLE stdout ERROR_VARIABLE stderr
  TIMEOUT 30)
if("${rejected}" STREQUAL "0" OR
   NOT stderr MATCHES "must be empty")
  message(FATAL_ERROR "Phase 3B2B1 existing-file protection not enforced:\n${stdout}\n${stderr}")
endif()
file(SHA256 "${OUTPUT_DIR}/headless_a/PhotonMap.dat" after_hash)
if(NOT "${initial_hash}" STREQUAL "${after_hash}")
  message(FATAL_ERROR "Phase 3B2B1 existing photon output was modified")
endif()
message(STATUS "Phase 3B2B1 production File exporter repeatability passed: ${parity_out}")
message(STATUS "Phase 3B2B1 no-overwrite preflight passed; GUI script is copied to:")
message(STATUS "${OUTPUT_DIR}/script/m3_exit_phase3b2b1_gui.tnhpps")
