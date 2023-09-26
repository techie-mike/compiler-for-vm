enable_testing()

include_directories(
    ${ROOT_COMPILER}
)

add_executable(
    graph_tests
    graph_tests.cpp
    ${COMPILER_SOURCES}
)

target_include_directories(graph_tests PUBLIC "${CMAKE_SOURCE_DIR}/src")
target_link_libraries(
    graph_tests
    GTest::gtest_main
)


include(GoogleTest)
gtest_discover_tests(graph_tests)

add_custom_target(
    graph_tests_gtest
    COMMAND graph_tests)
