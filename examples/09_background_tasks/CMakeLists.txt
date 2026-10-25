cmake_minimum_required(VERSION 3.20)
project(moji_example_09_background_tasks LANGUAGES CXX)

add_subdirectory(../.. "${CMAKE_CURRENT_BINARY_DIR}/moji")

add_executable(09_background_tasks main.cpp)
target_link_libraries(09_background_tasks PRIVATE moji::core)
