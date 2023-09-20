enable_testing()

add_executable(
  graph_tests
  graph_tests.cpp
)
target_include_directories(graph_tests PUBLIC "${CMAKE_SOURCE_DIR}/src")
target_link_libraries(
  graph_tests
  GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(graph_tests)
