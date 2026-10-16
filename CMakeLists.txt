cmake_minimum_required(VERSION 3.16)
project(MicroArchSim
    VERSION 1.0.0
    DESCRIPTION "Cycle-Accurate CPU Pipeline and Memory Hierarchy Simulator"
    LANGUAGES CXX
)

# C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler warnings
if(MSVC)
    add_compile_options(/W4 /WX- /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion)
endif()

# Optional sanitizers (Linux/macOS only)
option(ENABLE_SANITIZERS "Enable ASan and UBSan" OFF)
if(ENABLE_SANITIZERS AND NOT MSVC)
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
endif()

# ============================================================================
# Main library (all simulator components)
# ============================================================================
add_library(microarchsim_lib STATIC
    src/instruction.cpp
    src/register_file.cpp
    src/memory.cpp
    src/cache.cpp
    src/hazard_unit.cpp
    src/pipeline.cpp
    src/reference_interpreter.cpp
    src/assembler.cpp
    src/statistics.cpp
)
target_include_directories(microarchsim_lib PUBLIC include)

# ============================================================================
# Main executable
# ============================================================================
add_executable(microarchsim src/main.cpp)
target_link_libraries(microarchsim PRIVATE microarchsim_lib)

# Set output directory
set_target_properties(microarchsim PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

# ============================================================================
# Tests (GoogleTest)
# ============================================================================
option(BUILD_TESTS "Build unit tests" ON)
if(BUILD_TESTS)
    # Fetch GoogleTest
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
    )
    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    enable_testing()
    add_subdirectory(tests)
endif()
