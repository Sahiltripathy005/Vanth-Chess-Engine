cmake_minimum_required(VERSION 3.15)
project(ChessEngine VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable compiler warnings
if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

# Source and include files
file(GLOB_RECURSE SOURCES 
    "src/*.cpp"
)

# Header files are in include/
include_directories(include)

# Define static library for core chess logic
add_library(chess_core STATIC ${SOURCES})

# Target include directories for consumers of chess_core
target_include_directories(chess_core PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

# --- GoogleTest Integration ---
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/heads/main.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_subdirectory(tests)
add_subdirectory(benchmarks)
add_subdirectory(tools/compare)

