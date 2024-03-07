cmake_minimum_required(VERSION 3.16)

project(Compiler)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_FLAGS "-pedantic -Wall -Wextra -Werror -Wshadow -g")

set(ROOT_COMPILER ${CMAKE_SOURCE_DIR})
set(COMPILER_SOURCES
    ${CMAKE_SOURCE_DIR}/src/graph.cpp
    ${CMAKE_SOURCE_DIR}/src/inst.cpp
    ${CMAKE_SOURCE_DIR}/src/utils/utils.cpp
    ${CMAKE_SOURCE_DIR}/src/optimizations/analysis/rpo.cpp
    ${CMAKE_SOURCE_DIR}/src/optimizations/analysis/analysis.cpp
    ${CMAKE_SOURCE_DIR}/src/optimizations/analysis/domtree.cpp
    ${CMAKE_SOURCE_DIR}/src/optimizations/analysis/loop_analysis.cpp
    ${CMAKE_SOURCE_DIR}/src/optimizations/gcm.cpp
    ${CMAKE_SOURCE_DIR}/src/optimizations/analysis/linear_order.cpp
    ${CMAKE_SOURCE_DIR}/src/optimizations/analysis/liveness_analyzer.cpp
    ${CMAKE_SOURCE_DIR}/src/optimizations/linear_scan.cpp
    ${CMAKE_SOURCE_DIR}/src/optimizations/peepholes.cpp
    ${CMAKE_SOURCE_DIR}/src/optimizations/constant_folding.cpp
)

add_library(CompilerLibBase ${COMPILER_SOURCES})
target_include_directories(CompilerLibBase PUBLIC "${CMAKE_SOURCE_DIR}/src")

add_subdirectory(tests)

include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)
