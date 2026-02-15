cmake_minimum_required(VERSION 3.20)
project(MatchingEngine VERSION 0.1.0 LANGUAGES CXX)

# Use C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler warnings — treat them as learning opportunities
add_compile_options(-Wall -Wextra -Wpedantic)

# Main library (so both main and tests can use it)
add_library(matching_engine_lib
    src/OrderBook.cpp
    src/MatchingEngine.cpp
)
target_include_directories(matching_engine_lib PUBLIC include)

# Main executable
add_executable(matching_engine src/main.cpp)
target_link_libraries(matching_engine PRIVATE matching_engine_lib)

# Benchmark executable
add_executable(benchmark src/benchmark.cpp)
target_link_libraries(benchmark PRIVATE matching_engine_lib)

# Simple test executable
add_executable(tests tests/test_matching.cpp)
target_link_libraries(tests PRIVATE matching_engine_lib)
