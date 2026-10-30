cmake_minimum_required(VERSION 3.20)
project(moji_example_02_hello_world LANGUAGES CXX)

add_subdirectory(../.. "${CMAKE_CURRENT_BINARY_DIR}/moji")

add_executable(02_hello_world main.cpp)
target_link_libraries(02_hello_world PRIVATE moji::core)
