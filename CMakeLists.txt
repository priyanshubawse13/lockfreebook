cmake_minimum_required(VERSION 3.20)
project(lockfreebook LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Sanitizers (debug builds)
option(ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(ENABLE_TSAN "Enable ThreadSanitizer" OFF)
if(ENABLE_ASAN)
  add_compile_options(-fsanitize=address,undefined)
  add_link_options(-fsanitize=address,undefined)
endif()
if(ENABLE_TSAN)
  add_compile_options(-fsanitize=thread)
  add_link_options(-fsanitize=thread)
endif()

include_directories(include)

# Core library
add_library(orderbook_lib src/order_book.cpp)
target_include_directories(orderbook_lib PUBLIC include)

# Main simulation binary
add_executable(lockfreebook_sim src/main.cpp)
target_link_libraries(lockfreebook_sim PRIVATE orderbook_lib)

# Benchmark binary
add_executable(lockfreebook_bench bench/bench.cpp)
target_link_libraries(lockfreebook_bench PRIVATE orderbook_lib)

# Tests (GoogleTest fetched via FetchContent)
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()
add_executable(lockfreebook_test tests/test_order_book.cpp)
target_link_libraries(lockfreebook_test PRIVATE orderbook_lib GTest::gtest_main)
include(GoogleTest)
gtest_discover_tests(lockfreebook_test)
