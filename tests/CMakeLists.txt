cmake_minimum_required(VERSION 3.26)
project(MasharifTests)

include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.17.0
)
# For Windows: prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(unit_tests
    LayoutTest.cpp
    FlexTests.cpp
    AlignmentTests.cpp
    DimensionTests.cpp
    SpacingTests.cpp
    GapTests.cpp
    BenchmarkTests.cpp
    ComputedMarginTests.cpp
    OutOfFlowRepositionTests.cpp
)

target_link_libraries(unit_tests  GTest::gtest_main masharif::masharifcore)

# Match masharifcore's IPO/LTO so the consumer link performs /LTCG: this is what actually
# realizes cross-TU inlining of the (STATIC) masharifcore /GL objects into the benchmark.
include(CheckIPOSupported)
check_ipo_supported(RESULT _ipo_supported)
if (_ipo_supported)
    set_target_properties(unit_tests PROPERTIES
            INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE
            INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO TRUE)
endif ()

include(GoogleTest)
gtest_discover_tests(unit_tests)
