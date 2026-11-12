# The unit suites contain only ordinary TEST() cases. Source scanning permits
# labels to be assigned directly as a real CTest list at configure time;
# gtest_discover_tests() escaped multi-label arguments differently in
# CMake 3.31 and 4.4, causing platform-specific CI failures.
add_executable(tonatiuhpp_math_tests
  IntervalTests.cpp
  "${CMAKE_SOURCE_DIR}/libraries/math/1D/Interval.cpp"
  "${CMAKE_SOURCE_DIR}/libraries/math/gcf.cpp"
)

target_compile_definitions(tonatiuhpp_math_tests
  PRIVATE
    TONATIUH_LIBRARIES_EXPORT
)

target_include_directories(tonatiuhpp_math_tests
  PRIVATE
    "${CMAKE_SOURCE_DIR}"
    "${CMAKE_SOURCE_DIR}/libraries"
)

target_link_libraries(tonatiuhpp_math_tests
  PRIVATE
    GTest::gtest_main
    Qt6::Core
)

if(MSVC)
  target_compile_options(tonatiuhpp_math_tests PRIVATE /permissive- /Zc:__cplusplus)
endif()

gtest_add_tests(TARGET tonatiuhpp_math_tests
  SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/IntervalTests.cpp"
  TEST_PREFIX unit.math.
  TEST_LIST _tonatiuhpp_interval_cases
)
set_tests_properties(${_tonatiuhpp_interval_cases} PROPERTIES
  LABELS "unit;math;fast"
)

add_executable(tonatiuhpp_math_box2d_tests
  Box2DTests.cpp
  "${CMAKE_SOURCE_DIR}/libraries/math/2D/Box2D.cpp"
  "${CMAKE_SOURCE_DIR}/libraries/math/2D/vec2d.cpp"
  "${CMAKE_SOURCE_DIR}/libraries/math/gcf.cpp"
)

target_compile_definitions(tonatiuhpp_math_box2d_tests
  PRIVATE
    TONATIUH_LIBRARIES_EXPORT
)

target_include_directories(tonatiuhpp_math_box2d_tests
  PRIVATE
    "${CMAKE_SOURCE_DIR}"
    "${CMAKE_SOURCE_DIR}/libraries"
)

target_link_libraries(tonatiuhpp_math_box2d_tests
  PRIVATE
    GTest::gtest_main
    Qt6::Core
)

if(MSVC)
  target_compile_options(tonatiuhpp_math_box2d_tests PRIVATE /permissive- /Zc:__cplusplus)
endif()

gtest_add_tests(TARGET tonatiuhpp_math_box2d_tests
  SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/Box2DTests.cpp"
  TEST_PREFIX unit.math.
  TEST_LIST _tonatiuhpp_box2d_cases
)
set_tests_properties(${_tonatiuhpp_box2d_cases} PROPERTIES
  LABELS "unit;math;fast"
)

# TONATIUHPP_P1_CENTRAL_WIN_RUNTIME: test EXEs inherit the one build/bin/<config> output.
