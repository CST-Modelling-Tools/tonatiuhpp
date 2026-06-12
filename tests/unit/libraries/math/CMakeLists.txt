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

gtest_discover_tests(tonatiuhpp_math_tests
  TEST_PREFIX unit.math.
  PROPERTIES LABELS "unit;math"
)
