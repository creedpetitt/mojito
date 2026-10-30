cmake_minimum_required(VERSION 3.21)
project(moji_example_02_https LANGUAGES CXX)

if(NOT moji_ENABLE_TLS)
  message(FATAL_ERROR "02_https requires -Dmoji_ENABLE_TLS=ON and an OpenSSL installation or the vcpkg TLS feature.")
endif()

if(NOT TARGET moji::core)
  set(moji_BUILD_EXAMPLES OFF)
  add_subdirectory(../.. "${CMAKE_CURRENT_BINARY_DIR}/moji")
endif()

add_executable(02_https main.cpp)
target_link_libraries(02_https PRIVATE moji::core)
