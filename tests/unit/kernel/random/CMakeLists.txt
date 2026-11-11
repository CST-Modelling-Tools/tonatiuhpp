set(_tonatiuhpp_gtest_discovery_mode POST_BUILD)
if(WIN32)
  set(_tonatiuhpp_gtest_discovery_mode PRE_TEST)
endif()

add_executable(tonatiuhpp_standard_random_tests
  StandardRandomTests.cpp
  "${CMAKE_SOURCE_DIR}/kernel/random/StandardRandom.cpp"
)

target_compile_definitions(tonatiuhpp_standard_random_tests PRIVATE TONATIUH_KERNEL_EXPORT)
target_include_directories(tonatiuhpp_standard_random_tests PRIVATE "${CMAKE_SOURCE_DIR}")
target_link_libraries(tonatiuhpp_standard_random_tests
  PRIVATE
    GTest::gtest_main
    Qt6::Core
)

if(MSVC)
  target_compile_options(tonatiuhpp_standard_random_tests PRIVATE /permissive- /Zc:__cplusplus)
endif()

gtest_discover_tests(tonatiuhpp_standard_random_tests
  TEST_PREFIX unit.random.
  DISCOVERY_MODE ${_tonatiuhpp_gtest_discovery_mode}
  PROPERTIES LABELS "unit;random"
)

# TONATIUHPP_P1_CENTRAL_WIN_RUNTIME: test EXEs inherit the one build/bin/<config> output.
