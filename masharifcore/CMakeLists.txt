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

# Enable whole-program optimization (LTO/IPO) for optimized configs. The target property
# is INTERPROCEDURAL_OPTIMIZATION — the CMAKE_-prefixed name is the *variable* that seeds
# it, so setting `CMAKE_INTERPROCEDURAL_OPTIMIZATION` as a target property (as before) was a
# silent no-op (zero /GL, zero /LTCG ever emitted). Per-config so Debug stays fast to link.
# masharifcore is STATIC, so its /GL objects are only combined at the consumer link — the
# consumer targets (unit_tests, example) set the same property to pull in /LTCG.
check_ipo_supported(RESULT _ipo_supported OUTPUT _ipo_message)
if (_ipo_supported)
    set_target_properties(masharifcore PROPERTIES
            INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE
            INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO TRUE)
else ()
    message(STATUS "masharifcore: IPO/LTO not supported: ${_ipo_message}")
endif ()

# NOTE: fast-math is intentionally NOT enabled. Combined with whole-program optimization
# (LTO/IPO above) it reassociates floating-point across function boundaries, which shifts
# computed positions past ASSERT_FLOAT_EQ tolerance AND flips the exact `==` cache-hit
# comparisons in Node::LayoutImpl (changing StrategyRuns) — it broke 3 covered FlexTests.
# Standard /fp:precise keeps layout results deterministic across the LTO boundary.

# Per-config -O flags come from CMake's own defaults (MSVC Release = /O2 /Ob2 /DNDEBUG;
# GCC/Clang Release = -O3 -DNDEBUG). The former explicit `if(CMAKE_BUILD_TYPE STREQUAL
# "Release") /O2 /Ob2` block was redundant on MSVC, a downgrade (-O2 vs -O3) on GCC/Clang,
# and never fired under multi-config generators (empty CMAKE_BUILD_TYPE) — removed.


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