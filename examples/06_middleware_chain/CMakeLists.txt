cmake_minimum_required(VERSION 3.20)
project(moji_example_06_middleware_chain LANGUAGES CXX)

add_subdirectory(../.. "${CMAKE_CURRENT_BINARY_DIR}/moji")

add_executable(06_middleware_chain main.cpp)
target_link_libraries(06_middleware_chain PRIVATE moji::core)
