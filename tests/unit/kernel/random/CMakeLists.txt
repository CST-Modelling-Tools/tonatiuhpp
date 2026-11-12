# The unit suites contain only ordinary TEST() cases. Source scanning permits
# labels to be assigned directly as a real CTest list at configure time;
# gtest_discover_tests() escaped multi-label arguments differently in
# CMake 3.31 and 4.4, causing platform-specific CI failures.
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

gtest_add_tests(TARGET tonatiuhpp_standard_random_tests
  SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/StandardRandomTests.cpp"
  TEST_PREFIX unit.random.
  TEST_LIST _tonatiuhpp_random_cases
)
set_tests_properties(${_tonatiuhpp_random_cases} PROPERTIES
  LABELS "unit;random;fast"
)

# TONATIUHPP_P1_CENTRAL_WIN_RUNTIME: test EXEs inherit the one build/bin/<config> output.
