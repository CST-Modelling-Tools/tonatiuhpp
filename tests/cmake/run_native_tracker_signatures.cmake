# Real executable/process scientific characterization of the one-axis tracker.
# This protects the opt-in vs legacy distinction without touching ray physics.
foreach(_required IN ITEMS TEST_EXECUTABLE TEST_WORKING_DIRECTORY SCENE_FILE OUTPUT_DIR)
  if(NOT DEFINED ${_required} OR "${${_required}}" STREQUAL "")
    message(FATAL_ERROR "Missing required ${_required}")
  endif()
endforeach()

file(MAKE_DIRECTORY "${OUTPUT_DIR}")
set(_signature_fields
  schema mode scene_sha256 rays seed grid_width grid_height
  aperture_area_bits irradiance_bits power_per_ray_bits
  hit_count front_hit_count hit_sha256
)
set(_identity_fields scene_sha256 rays seed grid_width grid_height irradiance_bits)

# Separate native CLI launches exercise plugin loading, scene loading, tracker
# lifecycle, fixed chunk seeding, actual hit callback and JSON persistence.
foreach(_run IN ITEMS tracked_one tracked_two legacy_one legacy_two)
  set(_output "${OUTPUT_DIR}/${_run}.json")
  file(REMOVE "${_output}")
  file(TO_CMAKE_PATH "${_output}" _diagnostic_path)

  set(_extra_args)
  if(_run MATCHES "^tracked_")
    set(_tracked TRUE)
    list(APPEND _extra_args --update-trackers)
  else()
    set(_tracked FALSE)
  endif()

  execute_process(
    COMMAND "${CMAKE_COMMAND}" -E env
      "TONATIUHPP_A0_TRACE_SIGNATURE_FILE=${_diagnostic_path}"
      "${TEST_EXECUTABLE}" --headless trace-scene "${SCENE_FILE}"
      --rays 20001 --seed 123456789 --no-export ${_extra_args}
    WORKING_DIRECTORY "${TEST_WORKING_DIRECTORY}"
    RESULT_VARIABLE _exit_code
    OUTPUT_VARIABLE _stdout
    ERROR_VARIABLE _stderr
    TIMEOUT 120
  )
  string(FIND "${_stdout}" "Trace completed." _completed_at)
  string(FIND "${_stdout}" "a0_headless_signature:" _signature_at)
  string(FIND "${_stdout}" "tracker_update: enabled" _tracker_update_at)
  if(NOT "${_exit_code}" STREQUAL "0"
      OR _completed_at EQUAL -1 OR _signature_at EQUAL -1
      OR (_tracked AND _tracker_update_at EQUAL -1)
      OR (NOT _tracked AND NOT _tracker_update_at EQUAL -1))
    message(STATUS "${_run} stdout:\n${_stdout}")
    message(STATUS "${_run} stderr:\n${_stderr}")
    message(FATAL_ERROR "Native tracker CLI run ${_run} failed or did not honor the tracker flag (exit ${_exit_code}).")
  endif()

  if(NOT EXISTS "${_output}")
    message(FATAL_ERROR "Native tracker CLI run ${_run} produced no signature: ${_output}")
  endif()
  file(READ "${_output}" _json)
  foreach(_field IN LISTS _signature_fields)
    string(JSON _value ERROR_VARIABLE _json_error GET "${_json}" "${_field}")
    if(NOT "${_json_error}" STREQUAL "NOTFOUND")
      message(FATAL_ERROR "Invalid JSON signature from ${_run}, field ${_field}: ${_json_error}")
    endif()
    set("_sig_${_run}_${_field}" "${_value}")
  endforeach()

  if(NOT "${_sig_${_run}_schema}" STREQUAL "tonatiuhpp.a0.native-hits.v1"
      OR NOT "${_sig_${_run}_mode}" STREQUAL "headless-cli"
      OR NOT "${_sig_${_run}_rays}" STREQUAL "20001"
      OR NOT "${_sig_${_run}_seed}" STREQUAL "123456789"
      OR NOT "${_sig_${_run}_grid_width}" STREQUAL "200"
      OR NOT "${_sig_${_run}_grid_height}" STREQUAL "200"
      OR NOT "${_sig_${_run}_hit_count}" MATCHES "^[1-9][0-9]*$")
    message(FATAL_ERROR "Native tracker CLI run ${_run} has unexpected trace identity or no hits.")
  endif()

  foreach(_digest IN ITEMS scene_sha256 hit_sha256)
    string(LENGTH "${_sig_${_run}_${_digest}}" _digest_length)
    if(NOT _digest_length EQUAL 64 OR
        NOT "${_sig_${_run}_${_digest}}" MATCHES "^[0-9a-f]+$")
      message(FATAL_ERROR "Native tracker CLI run ${_run}: invalid ${_digest}.")
    endif()
  endforeach()
endforeach()

# Identical fixed seeds, but distinct processes. Do not include timings,
# worker counts or order of callbacks in scientific equality checks.
foreach(_mode IN ITEMS tracked legacy)
  foreach(_field IN LISTS _signature_fields)
    if(NOT "${_sig_${_mode}_one_${_field}}" STREQUAL "${_sig_${_mode}_two_${_field}}")
      message(FATAL_ERROR "Native CLI ${_mode} is not cross-process repeatable: ${_field} differs.")
    endif()
  endforeach()
endforeach()

# Tracked vs legacy must use the same on-disk scene, rays, seed and solar
# sampling grid. The tracker rotation may legitimately alter aperture bounds,
# counts and energy normalization, so do not require those to agree.
foreach(_field IN LISTS _identity_fields)
  if(NOT "${_sig_tracked_one_${_field}}" STREQUAL "${_sig_legacy_one_${_field}}")
    message(FATAL_ERROR "Native tracked and legacy runs do not share ${_field}.")
  endif()
endforeach()
if("${_sig_tracked_one_hit_sha256}" STREQUAL "${_sig_legacy_one_hit_sha256}")
  message(FATAL_ERROR "Native tracked and legacy paths have identical exact-hit digests: tracker update did not change optics.")
endif()

message(STATUS "Native tracked and legacy CLI runs are independently repeatable (20001 rays; 123456789 seed).")
message(STATUS "Tracked digest: ${_sig_tracked_one_hit_sha256}")
message(STATUS "Legacy digest:  ${_sig_legacy_one_hit_sha256} (scientifically different, as expected)")
