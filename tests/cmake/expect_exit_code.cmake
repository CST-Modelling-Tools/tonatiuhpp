if(NOT DEFINED TEST_EXECUTABLE OR TEST_EXECUTABLE STREQUAL "")
  message(FATAL_ERROR "TEST_EXECUTABLE is required.")
endif()

if(NOT DEFINED EXPECTED_EXIT_CODE OR EXPECTED_EXIT_CODE STREQUAL "")
  message(FATAL_ERROR "EXPECTED_EXIT_CODE is required.")
endif()

set(_test_arguments)
if(DEFINED TEST_ARGUMENTS AND NOT TEST_ARGUMENTS STREQUAL "")
  string(REPLACE "|" ";" _test_arguments "${TEST_ARGUMENTS}")
endif()

set(_working_directory)
if(DEFINED TEST_WORKING_DIRECTORY AND NOT TEST_WORKING_DIRECTORY STREQUAL "")
  set(_working_directory WORKING_DIRECTORY "${TEST_WORKING_DIRECTORY}")
endif()

execute_process(
  COMMAND "${TEST_EXECUTABLE}" ${_test_arguments}
  ${_working_directory}
  RESULT_VARIABLE _exit_code
  OUTPUT_VARIABLE _stdout
  ERROR_VARIABLE _stderr
)

if(NOT "${_exit_code}" STREQUAL "${EXPECTED_EXIT_CODE}")
  message(STATUS "stdout:\n${_stdout}")
  message(STATUS "stderr:\n${_stderr}")
  message(FATAL_ERROR "Expected exit code ${EXPECTED_EXIT_CODE}, got ${_exit_code}.")
endif()
