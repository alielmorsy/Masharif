cmake_minimum_required(VERSION 3.26)
project(example)
add_executable(test main.cpp)
target_link_libraries(test PRIVATE masharifcore)

target_include_directories(test PRIVATE ../)