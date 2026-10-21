if(NOT DEFINED TEST_EXECUTABLE OR TEST_EXECUTABLE STREQUAL "")
  message(FATAL_ERROR "TEST_EXECUTABLE is required.")
endif()
if(NOT DEFINED OUTPUT_DIR OR OUTPUT_DIR STREQUAL "")
  message(FATAL_ERROR "OUTPUT_DIR is required.")
endif()

file(MAKE_DIRECTORY "${OUTPUT_DIR}")
file(TO_CMAKE_PATH "${OUTPUT_DIR}/scene_document_ok.tnhpps" _ok)
file(TO_CMAKE_PATH "${OUTPUT_DIR}/scene_document_bad.tnhpps" _bad)
file(WRITE "${_ok}" "tn.Clear();\nvar node = new NodeObject;\nnode.setName('M3_2_Headless_Document');\nnode.createNode('child');\ntn.InsertScene(node);\nif (!tn.getScene().getPart('world.sun.position')) throw new Error('Missing sun');\nprint('M3.2 headless document passed');\n")
file(WRITE "${_bad}" "tn.Clear();\ntn.InsertScene({invalid: true});\n")

set(_working_directory)
if(DEFINED TEST_WORKING_DIRECTORY AND NOT TEST_WORKING_DIRECTORY STREQUAL "")
  set(_working_directory WORKING_DIRECTORY "${TEST_WORKING_DIRECTORY}")
endif()

execute_process(
  COMMAND "${TEST_EXECUTABLE}" --headless run-script "${_ok}"
  ${_working_directory}
  RESULT_VARIABLE _ok_exit
  OUTPUT_VARIABLE _ok_stdout
  ERROR_VARIABLE _ok_stderr
)
if(NOT "${_ok_exit}" STREQUAL "0" OR NOT _ok_stdout MATCHES "M3.2 headless document passed")
  message(STATUS "stdout:\n${_ok_stdout}")
  message(STATUS "stderr:\n${_ok_stderr}")
  message(FATAL_ERROR "Headless document success case failed (${_ok_exit}).")
endif()

execute_process(
  COMMAND "${TEST_EXECUTABLE}" --headless run-script "${_bad}"
  ${_working_directory}
  RESULT_VARIABLE _bad_exit
  OUTPUT_VARIABLE _bad_stdout
  ERROR_VARIABLE _bad_stderr
)
if(NOT "${_bad_exit}" STREQUAL "1"
    OR NOT _bad_stderr MATCHES "tn\\.InsertScene requires a NodeObject"
    OR NOT _bad_stderr MATCHES "Script execution failed: [^\r\n]*scene_document_bad\\.tnhpps:2")
  message(STATUS "stdout:\n${_bad_stdout}")
  message(STATUS "stderr:\n${_bad_stderr}")
  message(FATAL_ERROR "Headless document validation-error case failed (${_bad_exit}).")
endif()
