cmake_minimum_required(VERSION 3.26)
project(browser_layout_check)

# Same-directory dump-tree-as-JSON harness for tools/browser_layout_check's browser
# cross-check. See README.md in this directory for the full workflow.
add_executable(browser_layout_harness harness.cpp)
target_link_libraries(browser_layout_harness PRIVATE masharif::masharifcore)

find_program(NODE_EXECUTABLE node)
if (NOT NODE_EXECUTABLE)
    message(STATUS "browser_layout_check: node not found on PATH -- run_browser_layout_check "
                    "target will fail until it is (see this directory's README.md)")
endif ()

file(GLOB FIXTURE_FILES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/fixtures/*.html)

# Both JSON dumps are written into the source dir (not the build dir), matching how
# extract.mjs/compare.mjs already read/write them relative to their own location, and
# already .gitignore'd there. Not added to ALL: it needs `npm install && npx playwright
# install chromium` done once by hand first, and shouldn't fire on every ordinary build.
set(MASHARIF_RESULTS ${CMAKE_CURRENT_SOURCE_DIR}/masharif_results.json)
set(WEB_RESULTS ${CMAKE_CURRENT_SOURCE_DIR}/web_results.json)

# Re-runs only when the harness binary is rebuilt (i.e. masharifcore or harness.cpp
# actually changed) -- an OUTPUT-tracked command, not a PRE_BUILD/ALWAYS one.
add_custom_command(
    OUTPUT ${MASHARIF_RESULTS}
    COMMAND $<TARGET_FILE:browser_layout_harness> > ${MASHARIF_RESULTS}
    DEPENDS browser_layout_harness
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Dumping Masharif's computed layout for the browser cross-check"
    VERBATIM
)

# Re-runs only when a fixture or the extraction script changes -- spinning up headless
# Chromium per fixture is the genuinely expensive step here, so this is the one most
# worth not repeating on every invocation.
add_custom_command(
    OUTPUT ${WEB_RESULTS}
    COMMAND ${NODE_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/extract.mjs
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/extract.mjs ${FIXTURE_FILES}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Extracting browser ground truth via Playwright"
    VERBATIM
)

# Phony driver target: `cmake --build <dir> --target run_browser_layout_check`.
# compare.mjs itself always reruns (it's a cheap diff, and phony targets have no output
# to check anyway) -- but its two real inputs above are properly OUTPUT/DEPENDS-tracked,
# so neither the harness rerun nor the Playwright extraction repeats unless something
# they depend on actually changed.
add_custom_target(run_browser_layout_check
    COMMAND ${NODE_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/compare.mjs
    DEPENDS ${MASHARIF_RESULTS} ${WEB_RESULTS} ${CMAKE_CURRENT_SOURCE_DIR}/compare.mjs
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Comparing Masharif output against browser ground truth"
    VERBATIM
)
