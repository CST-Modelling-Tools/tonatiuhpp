set(_tonatiuhpp_equivalence_core_dir "${CMAKE_SOURCE_DIR}/application/core")

add_executable(tonatiuhpp_scientific_equivalence_tests
  TraceEquivalenceTests.cpp
  "${_tonatiuhpp_equivalence_core_dir}/CorePluginRegistry.cpp"
  "${_tonatiuhpp_equivalence_core_dir}/NativeTraceSignature.cpp"
  "${_tonatiuhpp_equivalence_core_dir}/RayTraceExecutor.cpp"
  "${_tonatiuhpp_equivalence_core_dir}/SceneInstanceBuilder.cpp"
  "${_tonatiuhpp_equivalence_core_dir}/SceneLoader.cpp"
  "${_tonatiuhpp_equivalence_core_dir}/TonatiuhCore.cpp"
  "${_tonatiuhpp_equivalence_core_dir}/TracePreparation.cpp"
)

target_include_directories(tonatiuhpp_scientific_equivalence_tests PRIVATE
  "${CMAKE_SOURCE_DIR}"
  "${CMAKE_SOURCE_DIR}/application"
  "${CMAKE_SOURCE_DIR}/libraries"
  "${COIN3D_INCLUDE_DIR}"
  "${SOQT_INCLUDE_DIR}"
)

target_compile_definitions(tonatiuhpp_scientific_equivalence_tests PRIVATE
  "TONATIUHPP_EQUIVALENCE_SCENE_FILE=\"${CMAKE_CURRENT_LIST_DIR}/../../examples/benchmarks/cylinder.tnhpp\""
)

target_compile_definitions(tonatiuhpp_scientific_equivalence_tests PRIVATE
  "TONATIUHPP_FRESNEL_SCENE_FILE=\"${CMAKE_CURRENT_LIST_DIR}/fixtures/fresnel-two-surface.tnhpp\""
)

# The fixture uses the real MaterialSpecular scene factory plugin.
add_dependencies(tonatiuhpp_scientific_equivalence_tests MaterialSpecular)

target_compile_definitions(tonatiuhpp_scientific_equivalence_tests PRIVATE
  "TONATIUHPP_SPECULAR_SCENE_FILE=\"${CMAKE_CURRENT_LIST_DIR}/fixtures/specular-two-reflections.tnhpp\""
  "TONATIUHPP_TRACKER_SCENE_FILE=\"${CMAKE_CURRENT_LIST_DIR}/fixtures/one-axis-tracker.tnhpp\""
  "TONATIUHPP_SCIENTIFIC_PLUGIN_DIRECTORY=\"${CMAKE_BINARY_DIR}/plugins\""
)

target_link_libraries(tonatiuhpp_scientific_equivalence_tests PRIVATE
  GTest::gtest
  Coin::Coin
  Qt6::Core
  Qt6::Gui
  Qt6::Concurrent
  TonatiuhLibraries
  TonatiuhKernel
)

if(MSVC)
  target_compile_options(tonatiuhpp_scientific_equivalence_tests PRIVATE /permissive- /Zc:__cplusplus)
endif()

set(_tonatiuhpp_equivalence_discovery POST_BUILD)
if(WIN32)
  set(_tonatiuhpp_equivalence_discovery PRE_TEST)
endif()

gtest_discover_tests(tonatiuhpp_scientific_equivalence_tests
  TEST_PREFIX scientific.
  DISCOVERY_MODE ${_tonatiuhpp_equivalence_discovery}
  PROPERTIES LABELS "scientific;equivalence" TIMEOUT 120
)

# A real Qt/SoQt application is needed to exercise MainWindow's sun-sizing
# method. Keep this exploratory diagnostic out of unattended CI.
option(TONATIUHPP_BUILD_GUI_SUN_DIAGNOSTIC
  "Build the opt-in Qt/SoQt sun-aperture comparison tool" OFF)

if(TONATIUHPP_BUILD_GUI_SUN_DIAGNOSTIC)
  add_executable(tonatiuhpp_gui_sun_diagnostic
    GuiSunSizingDiagnostic.cpp
    "${_tonatiuhpp_equivalence_core_dir}/CorePluginRegistry.cpp"
    "${_tonatiuhpp_equivalence_core_dir}/SceneInstanceBuilder.cpp"
    "${_tonatiuhpp_equivalence_core_dir}/SceneLoader.cpp"
    "${_tonatiuhpp_equivalence_core_dir}/TonatiuhCore.cpp"
  )
  target_include_directories(tonatiuhpp_gui_sun_diagnostic PRIVATE
    "${CMAKE_SOURCE_DIR}"
    "${CMAKE_SOURCE_DIR}/application"
    "${CMAKE_SOURCE_DIR}/libraries"
    "${COIN3D_INCLUDE_DIR}"
    "${SOQT_INCLUDE_DIR}"
  )
  target_compile_definitions(tonatiuhpp_gui_sun_diagnostic PRIVATE
    "TONATIUHPP_EQUIVALENCE_SCENE_FILE=\"${CMAKE_CURRENT_LIST_DIR}/../../examples/benchmarks/cylinder.tnhpp\""
    "TONATIUHPP_FRESNEL_SCENE_FILE=\"${CMAKE_CURRENT_LIST_DIR}/fixtures/fresnel-two-surface.tnhpp\""
  )
  target_link_libraries(tonatiuhpp_gui_sun_diagnostic PRIVATE
    Coin::Coin SoQt::SoQt Qt6::Core Qt6::Gui Qt6::Widgets
    TonatiuhLibraries TonatiuhKernel
  )
  if(MSVC)
    target_compile_options(tonatiuhpp_gui_sun_diagnostic PRIVATE /permissive- /Zc:__cplusplus)
  endif()
endif()
