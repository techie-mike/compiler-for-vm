enable_testing()

include_directories(
    ${ROOT_COMPILER}
)
set(ALL_LIBS_FOR_TESTS
    GTest::gtest_main
    CompilerLibBase)

add_executable(
    graph_tests
    graph_tests.cpp
    graph_comparator.cpp
)

target_include_directories(graph_tests PUBLIC "${CMAKE_SOURCE_DIR}/src")

target_link_libraries(
    graph_tests
    ${ALL_LIBS_FOR_TESTS}
)

add_executable(
    analysis_tests
    analysis_tests.cpp
    graph_comparator.cpp
)

target_include_directories(analysis_tests PUBLIC "${CMAKE_SOURCE_DIR}/src")
target_link_libraries(
    analysis_tests
    ${ALL_LIBS_FOR_TESTS}
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

add_executable(
    peepholes_tests
    peepholes_tests.cpp
    graph_comparator.cpp
)
target_include_directories(peepholes_tests PUBLIC "${CMAKE_SOURCE_DIR}/src")

gtest_discover_tests(peepholes_tests)

target_link_libraries(
    peepholes_tests
    ${ALL_LIBS_FOR_TESTS}
)

add_custom_target(
    peepholes_tests_gtest
    COMMAND peepholes_tests
)


add_custom_target(
    tests
    DEPENDS graph_tests_gtest analysis_tests_gtest peepholes_tests_gtest
)
