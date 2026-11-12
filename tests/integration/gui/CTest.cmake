# Registration is opt-in until a genuine Qt/SoQt/OpenGL display is verified on
# the intended runner. These exercise the REAL GUI executable, not a Qt stub.
if(NOT TARGET tonatiuhpp-gui OR NOT TARGET tonatiuhpp-cli)
  message(FATAL_ERROR "Native GUI acceptance needs production GUI and CLI targets")
endif()

set(_tonatiuhpp_gui_display_launcher)
if(TONATIUHPP_GUI_ACCEPTANCE_USE_XVFB)
  if(NOT UNIX OR APPLE)
    message(FATAL_ERROR "TONATIUHPP_GUI_ACCEPTANCE_USE_XVFB needs Linux/X11")
  endif()
  find_program(TONATIUHPP_XVFB_RUN_EXECUTABLE NAMES xvfb-run REQUIRED)
  list(APPEND _tonatiuhpp_gui_display_launcher
    "${TONATIUHPP_XVFB_RUN_EXECUTABLE}" -a -s "-screen 0 1280x1024x24 +extension GLX")
endif()

foreach(_mode IN ITEMS interactive-file seeded-interception interactive-flux)
  string(REPLACE "-" "_" _short_name "${_mode}")
  add_test(NAME "gui.native_${_short_name}"
    COMMAND ${_tonatiuhpp_gui_display_launcher}
      "${Python3_EXECUTABLE}"
      "${TONATIUHPP_TESTS_ROOT}/integration/gui/run_native_gui_acceptance.py"
      --mode "${_mode}"
      --gui "$<TARGET_FILE:tonatiuhpp-gui>"
      --cli "$<TARGET_FILE:tonatiuhpp-cli>"
      --source-root "${TONATIUHPP_TESTS_ROOT}/.."
      --output-root "${CMAKE_CURRENT_BINARY_DIR}/gui_native_acceptance")
  set_tests_properties("gui.native_${_short_name}" PROPERTIES
    LABELS "gui;scientific;system;regression"
    WORKING_DIRECTORY "$<TARGET_FILE_DIR:tonatiuhpp-gui>"
    RUN_SERIAL TRUE
    TIMEOUT 180)
endforeach()
