cmake_minimum_required(VERSION 3.26)
project(Masharif)

# With a single-config generator and no explicit build type, CMake emits no -O flag and
# silently produces an unoptimized binary. Default to Release so a bare configure is
# optimized; an explicit -DCMAKE_BUILD_TYPE=Debug is untouched.
get_property(_isMultiConfig GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if (NOT _isMultiConfig AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif ()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON) # never silently downgrade to an older standard
add_subdirectory(masharifcore)
add_subdirectory(example)
add_subdirectory(tests)