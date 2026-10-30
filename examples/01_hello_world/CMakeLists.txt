cmake_minimum_required(VERSION 3.21)
project(moji_example_01_hello_world LANGUAGES CXX)

if(NOT TARGET moji::core)
  set(moji_BUILD_EXAMPLES OFF)
  add_subdirectory(../.. "${CMAKE_CURRENT_BINARY_DIR}/moji")
endif()

add_executable(01_hello_world main.cpp)
target_link_libraries(01_hello_world PRIVATE moji::core)
