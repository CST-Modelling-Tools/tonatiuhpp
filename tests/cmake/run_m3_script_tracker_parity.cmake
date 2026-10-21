# M3.3 one-axis tracker scene-state and repeat-interception parity.
# TWO independent real headless processes; GUI check is separately required.
foreach(required IN ITEMS TEST_EXECUTABLE TEST_SCRIPT OUTPUT_DIR)
  if(NOT DEFINED ${required} OR "${${required}}" STREQUAL "")
    message(FATAL_ERROR "Missing ${required}")
  endif()
endforeach()
if(NOT EXISTS "${TEST_SCRIPT}")
  message(FATAL_ERROR "Missing tracker script ${TEST_SCRIPT}")
endif()
set(script_dir "${OUTPUT_DIR}/script")
set(other_dir "${OUTPUT_DIR}/elsewhere")
set(script_path "${script_dir}/m3_3_tracker_parity.tnhpps")
set(output_path "${script_dir}/m3_3_tracker_result.csv")
set(baseline_path "${OUTPUT_DIR}/headless_expected.csv")
file(MAKE_DIRECTORY "${script_dir}" "${other_dir}")
configure_file("${TEST_SCRIPT}" "${script_path}" COPYONLY)
file(REMOVE "${output_path}" "${baseline_path}" "${other_dir}/m3_3_tracker_result.csv")

foreach(attempt RANGE 1 2)
  execute_process(
    COMMAND "${TEST_EXECUTABLE}" --headless run-script "${script_path}"
    WORKING_DIRECTORY "${other_dir}"
    RESULT_VARIABLE exit_code OUTPUT_VARIABLE stdout_text ERROR_VARIABLE stderr_text
    TIMEOUT 60
  )
  if(NOT "${exit_code}" STREQUAL "0")
    message(FATAL_ERROR "M3.3 tracked script run ${attempt} failed (${exit_code}): ${stderr_text} ${stdout_text}")
  endif()
  if(NOT stdout_text MATCHES "M3[.]3 tracker parity fixture passed:")
    message(FATAL_ERROR "Missing tracked script success marker, run ${attempt}: ${stdout_text}")
  endif()
  if(NOT EXISTS "${output_path}" OR EXISTS "${other_dir}/m3_3_tracker_result.csv")
    message(FATAL_ERROR "Tracked artifact missing or written to process working directory")
  endif()
  file(READ "${output_path}" artifact)
  string(REPLACE "\r\n" "\n" artifact "${artifact}")
  string(REPLACE "\r" "\n" artifact "${artifact}")
  string(REGEX MATCHALL "[^\n]+" rows "${artifact}")
  list(LENGTH rows row_count)
  if(NOT row_count EQUAL 12)
    message(FATAL_ERROR "Expected 12 tracker result rows, got ${row_count}: ${artifact}")
  endif()
  list(GET rows 0 schema)
  list(GET rows 1 surface)
  list(GET rows 2 rays)
  list(GET rows 3 az1)
  list(GET rows 4 az2)
  list(GET rows 5 elevation)
  list(GET rows 6 enabled)
  list(GET rows 7 target)
  list(GET rows 8 angles1)
  list(GET rows 9 angles2)
  list(GET rows 10 power1)
  list(GET rows 11 power2)
  if(NOT schema STREQUAL "schema,m3.3-tracker-parity-v1"
      OR NOT surface STREQUAL "surface,//Node/M3_3_Tracking/Receiver/Shape"
      OR NOT rays STREQUAL "rays,2048"
      OR NOT az1 STREQUAL "sun_first_azimuth,90"
      OR NOT az2 STREQUAL "sun_second_azimuth,110"
      OR NOT elevation STREQUAL "sun_elevation,60"
      OR NOT enabled STREQUAL "tracker_enabled,true"
      OR NOT target STREQUAL "target_aiming_point,0 0 6"
      OR NOT angles1 MATCHES "^target_angles_first,.+"
      OR NOT angles2 MATCHES "^target_angles_second,.+"
      OR NOT power1 MATCHES "^power_first_w,[0-9]+([.][0-9]+)?([eE][+-]?[0-9]+)?$"
      OR NOT power2 MATCHES "^power_second_w,[0-9]+([.][0-9]+)?([eE][+-]?[0-9]+)?$")
    message(FATAL_ERROR "Malformed tracker parity artifact on run ${attempt}: ${artifact}")
  endif()
  if(angles1 STREQUAL angles2)
    message(FATAL_ERROR "Tracker angles did not change with sun azimuth")
  endif()
  if(attempt EQUAL 1)
    set(first_artifact "${artifact}")
    file(WRITE "${baseline_path}" "${artifact}")
  elseif(NOT artifact STREQUAL first_artifact)
    message(FATAL_ERROR "Tracked scientific artifacts differ across independent headless runs:
FIRST:
${first_artifact}
SECOND:
${artifact}")
  endif()
endforeach()

# Guard against false GUI acceptance from a stale headless-generated file.
file(REMOVE "${output_path}")
if(EXISTS "${output_path}")
  message(FATAL_ERROR "Could not clear the tracker output before GUI acceptance")
endif()
message(STATUS "M3.3 tracker headless baseline matched twice; GUI comparison pending")
message(STATUS "Baseline: ${baseline_path}")
message(STATUS "GUI script: ${script_path}")
