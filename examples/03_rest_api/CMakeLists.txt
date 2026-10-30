cmake_minimum_required(VERSION 3.21)
project(moji_example_03_rest_api LANGUAGES CXX)

if(NOT TARGET moji::core)
  set(moji_BUILD_EXAMPLES OFF)
  add_subdirectory(../.. "${CMAKE_CURRENT_BINARY_DIR}/moji")
endif()

add_executable(03_rest_api main.cpp users.cpp)
target_link_libraries(03_rest_api PRIVATE moji::core)
