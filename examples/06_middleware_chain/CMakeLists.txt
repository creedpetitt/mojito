cmake_minimum_required(VERSION 3.21)
project(moji_example_06_middleware_chain LANGUAGES CXX)

if(NOT TARGET moji::core)
  set(moji_BUILD_EXAMPLES OFF)
  add_subdirectory(../.. "${CMAKE_CURRENT_BINARY_DIR}/moji")
endif()

add_executable(06_middleware_chain main.cpp)
target_link_libraries(06_middleware_chain PRIVATE moji::core)
