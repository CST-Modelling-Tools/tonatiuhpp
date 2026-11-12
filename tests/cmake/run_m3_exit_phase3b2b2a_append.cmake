# M3 Phase 3B2B2A, real repeated File export, bounded 2 x 2048 rays.
cmake_minimum_required(VERSION 3.21)
foreach(v TEST_EXECUTABLE TEST_SCRIPT TEST_SCENE APPEND_VERIFIER COMPARATOR)
  if(NOT DEFINED ${v} OR "${${v}}" STREQUAL "" OR NOT EXISTS "${${v}}")
    message(FATAL_ERROR "Phase 3B2B2A missing ${v}: ${${v}}")
  endif()
endforeach()
if(NOT DEFINED OUTPUT_DIR OR OUTPUT_DIR STREQUAL "" OR
   NOT DEFINED TEST_BUILD_ROOT OR TEST_BUILD_ROOT STREQUAL "")
  message(FATAL_ERROR "Phase 3B2B2A requires its registered build/test output root")
endif()
get_filename_component(expected_output
  "${TEST_BUILD_ROOT}/m3_exit_phase3b2b2a_append" ABSOLUTE)
get_filename_component(actual_output "${OUTPUT_DIR}" ABSOLUTE)
file(TO_CMAKE_PATH "${expected_output}" expected_output)
file(TO_CMAKE_PATH "${actual_output}" actual_output)
if(NOT "${expected_output}" STREQUAL "${actual_output}")
  message(FATAL_ERROR "Phase 3B2B2A refuses non-fixture output path: ${OUTPUT_DIR}")
endif()
find_program(PHASE3B2B2A_PYTHON NAMES python3 python REQUIRED)

include("${CMAKE_CURRENT_LIST_DIR}/PrepareTestOutput.cmake")
tonatiuhpp_prepare_test_output("${OUTPUT_DIR}" "${TEST_BUILD_ROOT}" "m3_exit_phase3b2b2a_append")
file(MAKE_DIRECTORY "${OUTPUT_DIR}/script")
file(TO_CMAKE_PATH "${TEST_SCENE}" scene_path)
file(READ "${TEST_SCRIPT}" template)
foreach(side single append drift tamper gui)
  file(MAKE_DIRECTORY "${OUTPUT_DIR}/${side}")
  file(TO_CMAKE_PATH "${OUTPUT_DIR}/${side}" export_path)
  string(REPLACE "@SCENE_FILE@" "${scene_path}" content "${template}")
  string(REPLACE "@EXPORT_DIRECTORY@" "${export_path}" content "${content}")
  if(side STREQUAL "single")
    set(second_run "")
  elseif(side STREQUAL "drift")
    set(second_run "tn.SetPhotonBufferSize(8192);\ntn.Run();")
  elseif(side STREQUAL "tamper")
    set(second_run "tn.writeJson('${export_path}/PhotonMap_parameters.txt', {tampered: true});\ntn.Run();")
  else()
    set(second_run "tn.Run();")
  endif()
  string(REPLACE "@SECOND_RUN@" "${second_run}" content "${content}")
  file(WRITE "${OUTPUT_DIR}/script/m3_exit_phase3b2b2a_${side}.tnhpps" "${content}")
endforeach()

foreach(side single append)
  execute_process(
    COMMAND "${TEST_EXECUTABLE}"  run-script
      "${OUTPUT_DIR}/script/m3_exit_phase3b2b2a_${side}.tnhpps"
    WORKING_DIRECTORY "${OUTPUT_DIR}/script"
    RESULT_VARIABLE code OUTPUT_VARIABLE stdout ERROR_VARIABLE stderr
    TIMEOUT 90)
  if(NOT "${code}" STREQUAL "0" OR
     NOT stdout MATCHES "M3 Phase 3B2B2A cumulative File append passed")
    message(FATAL_ERROR "Phase 3B2B2A ${side} run failed (${code}):\n${stdout}\n${stderr}")
  endif()
  if(NOT EXISTS "${OUTPUT_DIR}/${side}/PhotonMap.dat" OR
     NOT EXISTS "${OUTPUT_DIR}/${side}/PhotonMap_parameters.txt")
    message(FATAL_ERROR "Phase 3B2B2A ${side}: production File output missing")
  endif()
endforeach()

execute_process(
  COMMAND "${PHASE3B2B2A_PYTHON}" "${APPEND_VERIFIER}"
    "${OUTPUT_DIR}/single" "${OUTPUT_DIR}/append"
  RESULT_VARIABLE compare_code OUTPUT_VARIABLE compare_out ERROR_VARIABLE compare_err
    TIMEOUT 30
)
if(NOT "${compare_code}" STREQUAL "0")
  message(FATAL_ERROR "Phase 3B2B2A real append mismatch:\n${compare_out}\n${compare_err}")
endif()

# Settings changes must not silently reuse a File exporter or corrupt output.
execute_process(
  COMMAND "${TEST_EXECUTABLE}"  run-script
    "${OUTPUT_DIR}/script/m3_exit_phase3b2b2a_drift.tnhpps"
  WORKING_DIRECTORY "${OUTPUT_DIR}/script"
  RESULT_VARIABLE drift_code OUTPUT_VARIABLE drift_out ERROR_VARIABLE drift_err
  TIMEOUT 60)
if("${drift_code}" STREQUAL "0" OR
   NOT drift_err MATCHES "cannot append after changing exporter/buffer settings")
  message(FATAL_ERROR "Phase 3B2B2A settings drift was not rejected:\n${drift_out}\n${drift_err}")
endif()
execute_process(
  COMMAND "${PHASE3B2B2A_PYTHON}" "${COMPARATOR}"
    "${OUTPUT_DIR}/single" "${OUTPUT_DIR}/drift"
  RESULT_VARIABLE drift_compare OUTPUT_VARIABLE drift_compare_out ERROR_VARIABLE drift_compare_err
    TIMEOUT 30
)
if(NOT "${drift_compare}" STREQUAL "0")
  message(FATAL_ERROR "Phase 3B2B2A rejected settings mutation changed photons:\n${drift_compare_out}\n${drift_compare_err}")
endif()

# Deliberate metadata tampering is confined to the disposable fixture folder.
# The second Run must reject before opening a File exporter.
file(SHA256 "${OUTPUT_DIR}/single/PhotonMap.dat" single_hash)
execute_process(
  COMMAND "${TEST_EXECUTABLE}"  run-script
    "${OUTPUT_DIR}/script/m3_exit_phase3b2b2a_tamper.tnhpps"
  WORKING_DIRECTORY "${OUTPUT_DIR}/script"
  RESULT_VARIABLE tamper_code OUTPUT_VARIABLE tamper_out ERROR_VARIABLE tamper_err
  TIMEOUT 60)
if("${tamper_code}" STREQUAL "0" OR
   NOT tamper_err MATCHES "existing File export changed externally")
  message(FATAL_ERROR "Phase 3B2B2A tamper guard not enforced:\n${tamper_out}\n${tamper_err}")
endif()
file(SHA256 "${OUTPUT_DIR}/tamper/PhotonMap.dat" tamper_data_hash)
if(NOT "${single_hash}" STREQUAL "${tamper_data_hash}")
  message(FATAL_ERROR "Phase 3B2B2A tamper guard allowed photon data mutation")
endif()
message(STATUS "${compare_out}")
message(STATUS "Phase 3B2B2A settings-drift and external-tamper rejection passed")
message(STATUS "Phase 3B2B2A GUI fixture: ${OUTPUT_DIR}/script/m3_exit_phase3b2b2a_gui.tnhpps")
