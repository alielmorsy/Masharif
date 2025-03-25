cmake_minimum_required(VERSION 3.26)
project(masharifcore)

include(CheckIPOSupported)
set(MASHARIF_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/..)
file(GLOB SOURCES CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/*.h
        ${CMAKE_CURRENT_SOURCE_DIR}/**/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/**/*.h)

add_library(masharifcore STATIC ${SOURCES})
check_ipo_supported(RESULT result)
if (result)
    set_target_properties(masharifcore PROPERTIES
            CMAKE_INTERPROCEDURAL_OPTIMIZATION true)
endif ()

if (MSVC)
    add_compile_options(/fp:fast)
elseif (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-ffast-math)
endif ()


if (CMAKE_BUILD_TYPE STREQUAL "Release")
    if (MSVC)
        add_compile_options(
                /O2
                /arch:AVX2
                /Ob2
        )
    elseif (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        add_compile_options(
                -O2
                -march=native
                -finline-functions
        )
    endif ()
endif ()


target_include_directories(masharifcore PRIVATE ${MASHARIF_ROOT})