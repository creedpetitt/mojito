cmake_minimum_required(VERSION 3.21)
project(moji_example_09_background_tasks LANGUAGES CXX)

if(NOT TARGET moji::core)
  set(moji_BUILD_EXAMPLES OFF)
  add_subdirectory(../.. "${CMAKE_CURRENT_BINARY_DIR}/moji")
endif()

add_executable(09_background_tasks main.cpp)
target_link_libraries(09_background_tasks PRIVATE moji::core)
