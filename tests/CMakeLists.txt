enable_testing()

include_directories(
    ${ROOT_COMPILER}
)

add_executable(
    graph_tests
    graph_tests.cpp
    graph_comparator.cpp
    ${COMPILER_SOURCES}
)

target_include_directories(graph_tests PUBLIC "${CMAKE_SOURCE_DIR}/src")

target_link_libraries(
    graph_tests
    GTest::gtest_main
)

add_executable(
    analysis_tests
    analysis_tests.cpp
    graph_comparator.cpp
    ${COMPILER_SOURCES}
)
target_include_directories(analysis_tests PUBLIC "${CMAKE_SOURCE_DIR}/src")
target_link_libraries(
    analysis_tests
    GTest::gtest_main
)


include(GoogleTest)
gtest_discover_tests(graph_tests)

add_custom_target(
    graph_tests_gtest
    COMMAND graph_tests
)

add_custom_target(
    analysis_tests_gtest
    COMMAND analysis_tests
)

add_custom_target(
    tests
    DEPENDS graph_tests_gtest analysis_tests_gtest
)
