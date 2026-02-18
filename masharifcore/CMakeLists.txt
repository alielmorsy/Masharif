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
add_library(masharif::masharifcore ALIAS masharifcore)

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
                /Ob2
        )
    elseif (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        add_compile_options(
                -O2
                -finline-functions
        )
    endif ()
endif ()


target_include_directories(masharifcore PUBLIC 
    $<BUILD_INTERFACE:${MASHARIF_ROOT}>
    $<INSTALL_INTERFACE:include>
)

include(GNUInstallDirs)

install(TARGETS masharifcore
    EXPORT masharifTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/masharifcore
    FILES_MATCHING PATTERN "*.h"
)

install(EXPORT masharifTargets
    FILE masharifTargets.cmake
    NAMESPACE masharif::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/masharif
)