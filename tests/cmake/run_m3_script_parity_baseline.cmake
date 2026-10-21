# M3.3: establish a stable native true-headless scientific artifact for the
# EXACT SAME script the user will then run in the real GUI Script Editor.
# Only the headless repeatability portion is automated here; GUI parity is
# a separate native acceptance gate (see docs/m3-script-parity-local.md).
foreach(required IN ITEMS TEST_EXECUTABLE TEST_SCRIPT OUTPUT_DIR)
  if(NOT DEFINED ${required} OR "${${required}}" STREQUAL "")
    message(FATAL_ERROR "Missing ${required}")
  endif()
endforeach()
if(NOT EXISTS "${TEST_SCRIPT}")
  message(FATAL_ERROR "M3.3 parity script is missing: ${TEST_SCRIPT}")
endif()

set(script_dir "${OUTPUT_DIR}/script")
set(other_dir "${OUTPUT_DIR}/elsewhere")
set(script_path "${script_dir}/m3_3_gui_headless_parity.tnhpps")
set(output_path "${script_dir}/m3_3_parity_result.csv")
set(baseline_path "${OUTPUT_DIR}/headless_expected.csv")
file(MAKE_DIRECTORY "${script_dir}" "${other_dir}")
configure_file("${TEST_SCRIPT}" "${script_path}" COPYONLY)
file(REMOVE "${output_path}" "${baseline_path}" "${other_dir}/m3_3_parity_result.csv")

foreach(attempt RANGE 1 2)
  execute_process(
    COMMAND "${TEST_EXECUTABLE}" --headless run-script "${script_path}"
    WORKING_DIRECTORY "${other_dir}"
    RESULT_VARIABLE exit_code OUTPUT_VARIABLE stdout_text ERROR_VARIABLE stderr_text
    TIMEOUT 45
  )
  if(NOT "${exit_code}" STREQUAL "0")
    message(FATAL_ERROR "M3.3 headless run ${attempt} failed (${exit_code}): ${stderr_text} ${stdout_text}")
  endif()
  string(REGEX MATCH "M3\\.3 parity fixture passed: ([0-9]+(\\.[0-9]+)?([eE][+-]?[0-9]+)?)"
    printed_line "${stdout_text}")
  if(printed_line STREQUAL "")
    message(FATAL_ERROR "M3.3 numerical success marker absent from headless run ${attempt}: ${stdout_text}")
  endif()
  set(printed_power "${CMAKE_MATCH_1}")
  if(NOT EXISTS "${output_path}")
    message(FATAL_ERROR "Script-relative scientific artifact not created: ${output_path}")
  endif()
  if(EXISTS "${other_dir}/m3_3_parity_result.csv")
    message(FATAL_ERROR "Script artifact incorrectly written to process working directory")
  endif()

  file(READ "${output_path}" artifact)
  string(REPLACE "\r\n" "\n" artifact "${artifact}")
  string(REPLACE "\r" "\n" artifact "${artifact}")
  string(REGEX MATCHALL "[^\n]+" rows "${artifact}")
  list(LENGTH rows row_count)
  if(NOT row_count EQUAL 6)
    message(FATAL_ERROR "Expected 6 scientific artifact rows, found ${row_count}: ${artifact}")
  endif()
  list(GET rows 0 schema_row)
  list(GET rows 1 surface_row)
  list(GET rows 2 rays_row)
  list(GET rows 3 azimuth_row)
  list(GET rows 4 elevation_row)
  list(GET rows 5 power_row)
  if(NOT schema_row STREQUAL "schema,m3.3-script-parity-v1"
      OR NOT surface_row STREQUAL "surface,//Node/M3_3_Parity/Receiver/Shape"
      OR NOT rays_row STREQUAL "rays,2048"
      OR NOT azimuth_row STREQUAL "sun_azimuth_deg,180"
      OR NOT elevation_row STREQUAL "sun_elevation_deg,90"
      OR NOT power_row MATCHES "^power_w,[0-9]+(\\.[0-9]+)?([eE][+-]?[0-9]+)?$")
    message(FATAL_ERROR "Invalid M3.3 scientific artifact from run ${attempt}: ${artifact}")
  endif()

  if(power_row MATCHES "^power_w,0+([.]0+)?$")
    message(FATAL_ERROR "M3.3 intercepted power must be strictly positive")
  endif()
  if(NOT power_row STREQUAL "power_w,${printed_power}")
    message(FATAL_ERROR "M3.3 printed power differs from artifact: ${printed_power} versus ${power_row}")
  endif()

  if(attempt EQUAL 1)
    set(first_artifact "${artifact}")
    file(WRITE "${baseline_path}" "${first_artifact}")
  elseif(NOT artifact STREQUAL first_artifact)
    message(FATAL_ERROR "M3.3 independent-process headless artifacts differ:\nFIRST:\n${first_artifact}\nSECOND:\n${artifact}")
  endif()
endforeach()

# The GUI must create its OWN artifact. Leaving the last headless artifact
# would let the GUI comparison falsely pass even if no GUI script ran.
file(REMOVE "${output_path}")
if(EXISTS "${output_path}")
  message(FATAL_ERROR "Could not clear headless output before GUI parity gate: ${output_path}")
endif()
message(STATUS "M3.3 bounded headless artifact repeatability passed: ${power_row}")
message(STATUS "Native GUI parity is still pending. Baseline: ${baseline_path}")
message(STATUS "GUI script to open: ${script_path}")
