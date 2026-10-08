set(_tonatiuhpp_equivalence_core_dir "${CMAKE_SOURCE_DIR}/application/core")

add_executable(tonatiuhpp_scientific_equivalence_tests
  TraceEquivalenceTests.cpp
  "${_tonatiuhpp_equivalence_core_dir}/CorePluginRegistry.cpp"
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
