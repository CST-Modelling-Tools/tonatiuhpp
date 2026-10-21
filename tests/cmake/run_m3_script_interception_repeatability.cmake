# M3.3 bounded native script repeatability.
# Identical scientific outputs across independent true-headless processes;
# this is not a substitute for the native GUI/CLI parity acceptance gate.
foreach(required IN ITEMS TEST_EXECUTABLE TEST_SCRIPT TEST_WORKING_DIRECTORY)
  if(NOT DEFINED ${required} OR "${${required}}" STREQUAL "")
    message(FATAL_ERROR "Missing ${required}")
  endif()
endforeach()

set(previous_power "")
foreach(attempt RANGE 1 2)
  execute_process(
    COMMAND "${TEST_EXECUTABLE}" --headless run-script "${TEST_SCRIPT}"
    WORKING_DIRECTORY "${TEST_WORKING_DIRECTORY}"
    RESULT_VARIABLE run_exit
    OUTPUT_VARIABLE run_output
    ERROR_VARIABLE run_error
    TIMEOUT 45
  )
  if(NOT "${run_exit}" STREQUAL "0")
    message(FATAL_ERROR "Script run ${attempt} failed (${run_exit}): ${run_error} ${run_output}")
  endif()

  string(REGEX MATCHALL "M3\\.2 bounded interception smoke passed: [0-9]+(\\.[0-9]+)?" matches "${run_output}")
  list(LENGTH matches count)
  if(NOT count EQUAL 1)
    message(FATAL_ERROR "Expected exactly one numerical interception result in run ${attempt}: ${run_output}")
  endif()
  list(GET matches 0 matched)
  string(REGEX REPLACE "^M3\\.2 bounded interception smoke passed: " "" power "${matched}")
  if(NOT power MATCHES "^[0-9]+(\\.[0-9]+)?$" OR power MATCHES "^0+(\\.0+)?$")
    message(FATAL_ERROR "Invalid or zero interception power on run ${attempt}: ${power}")
  endif()
  if(attempt EQUAL 2 AND NOT power STREQUAL previous_power)
    message(FATAL_ERROR "Scripted interception changed between processes: ${previous_power} versus ${power}")
  endif()
  set(previous_power "${power}")
endforeach()
message(STATUS "M3.3 bounded true-headless repeatability passed: ${previous_power} W")
