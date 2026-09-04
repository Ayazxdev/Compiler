cmake_minimum_required(VERSION 3.25)

project(
  Compiler
  VERSION 0.1.0
  DESCRIPTION "Compiler & Sandboxed Bytecode VM (Gravity) for the custom Luna programming language"
  LANGUAGES CXX
)

# C++ standard
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)   # -std=c++26, not -std=gnu++26

# Export compile commands (for clang-tidy / IDEs)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Dependencies via FetchContent
include(FetchContent)

# GoogleTest
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.15.2
  GIT_SHALLOW    TRUE
)
# Prevent overriding the parent project's compiler/linker settings on Windows
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Google Benchmark
FetchContent_Declare(
  benchmark
  GIT_REPOSITORY https://github.com/google/benchmark.git
  GIT_TAG        v1.9.1
  GIT_SHALLOW    TRUE
)
set(BENCHMARK_ENABLE_TESTING      OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_INSTALL      OFF CACHE BOOL "" FORCE)
set(BENCHMARK_DOWNLOAD_DEPENDENCIES ON CACHE BOOL "" FORCE)
# Work around the regex-backend detection failure on Windows/Clang:
# Google Benchmark tries to compile & run test programs; that fails in some
# cross-compilation or MinGW/LLVM environments.  Force it to use the STL backend.
set(HAVE_STD_REGEX         ON  CACHE BOOL "" FORCE)
set(HAVE_GNU_POSIX_REGEX   OFF CACHE BOOL "" FORCE)
set(HAVE_POSIX_REGEX       OFF CACHE BOOL "" FORCE)
set(HAVE_STEADY_CLOCK      ON  CACHE BOOL "" FORCE)
# Suppress Clang 22's -Wc2y-extensions triggered inside benchmark.h's __COUNTER__ use.
# This is a Google Benchmark v1.9.1 issue with bleeding-edge Clang — not my code.
set(BENCHMARK_CXX_WARNINGS "" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS_BENCHMARK_WORKAROUND "-Wno-c2y-extensions" CACHE STRING "" FORCE)
FetchContent_MakeAvailable(benchmark)

# Suppress the spurious Clang 22 warning on benchmark's own targets post-populate.
# (FetchContent_MakeAvailable has already defined the benchmark and benchmark_main targets.)
foreach(_bench_tgt IN ITEMS benchmark benchmark_main)
  if(TARGET ${_bench_tgt})
    target_compile_options(${_bench_tgt} PRIVATE
      $<$<CXX_COMPILER_ID:Clang,AppleClang>:-Wno-c2y-extensions -Wno-error>
    )
  endif()
endforeach()

# Compiler warning flags
# Applied to our own targets only, not to FetchContent'd libraries.
set(LUNA_WARN_FLAGS)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
  list(APPEND LUNA_WARN_FLAGS
    -Wall -Wextra -Wpedantic
    -Wshadow
    -Wconversion
    -Wsign-conversion
    -Wnull-dereference
    -Wdouble-promotion
    -Wformat=2
  )
  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang")
    list(APPEND LUNA_WARN_FLAGS
      # Suppress Weverything sub-warnings not useful for this project:
      -Wno-c++98-compat
      -Wno-c++98-compat-pedantic
      -Wno-pre-c++20-compat-pedantic
      -Wno-padded
      -Wno-exit-time-destructors
      -Wno-global-constructors
      -Wno-switch-enum   # allow default: in exhaustive switches
      -Wno-c2y-extensions  # Clang 22: __COUNTER__ extension, irrelevant for us
    )
  endif()
elseif(MSVC)
  list(APPEND LUNA_WARN_FLAGS /W4 /permissive-)
endif()

# Per-build-type optimization flags
# Applied to our own targets only, NOT via CMAKE_CXX_FLAGS (which would leak into FetchContent'd dependencies and cause build failures).
set(LUNA_OPT_FLAGS)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  list(APPEND LUNA_OPT_FLAGS -g -O0)
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
  list(APPEND LUNA_OPT_FLAGS -O3 -march=native -DNDEBUG)
elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
  list(APPEND LUNA_OPT_FLAGS -O2 -g -fno-omit-frame-pointer)
endif()

# Sanitizer helper
option(LUNA_SANITIZE "Enable ASan + UBSan" OFF)

set(LUNA_SANITIZE_FLAGS)
if(LUNA_SANITIZE)
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    list(APPEND LUNA_SANITIZE_FLAGS
      -fsanitize=address,undefined
      -fno-omit-frame-pointer
    )
  else()
    message(WARNING "LUNA_SANITIZE is only supported on GCC/Clang; ignoring.")
  endif()
endif()

# Sub-directories
add_subdirectory(src)

enable_testing()
add_subdirectory(tests)
add_subdirectory(bench)
