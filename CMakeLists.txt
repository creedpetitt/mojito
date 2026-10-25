cmake_minimum_required(VERSION 3.20)
project(moji VERSION 1.0.0 LANGUAGES CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(cmake/MojiDependencies.cmake)

if(PROJECT_IS_TOP_LEVEL)
  set(moji_DEFAULT_INSTALL ON)
else()
  set(moji_DEFAULT_INSTALL OFF)
endif()

option(
  moji_INSTALL "Generate install rules for moji" ${moji_DEFAULT_INSTALL})

moji_fetch_boost()
find_package(Threads REQUIRED)

if(NOT TARGET moji_boost_system)
  add_library(moji_boost_system STATIC
        ${moji_BOOST_SOURCE_DIR}/libs/system/src/error_code.cpp
    )
  target_include_directories(moji_boost_system PUBLIC
        $<BUILD_INTERFACE:${moji_BOOST_SOURCE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    )
  set_target_properties(
      moji_boost_system PROPERTIES EXPORT_NAME boost_system)
endif()

set(moji_SOURCES
    src/app.cpp
    src/config.cpp
    src/http_session.cpp
    src/json.cpp
    src/request.cpp
    src/dispatcher.cpp
    src/response.cpp
    src/router.cpp
    src/server/server.cpp
    src/server/listener.cpp
    src/signal_handler.cpp
    src/util/string.cpp
)

add_library(moji_core STATIC ${moji_SOURCES})
add_library(moji::core ALIAS moji_core)
set_target_properties(moji_core PROPERTIES EXPORT_NAME core)
target_compile_features(moji_core PUBLIC cxx_std_23)

target_include_directories(moji_core BEFORE PUBLIC
    $<BUILD_INTERFACE:${moji_BOOST_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(moji_core PUBLIC
    moji_boost_system
    Threads::Threads
)

if(moji_INSTALL)
  install(
        TARGETS moji_core moji_boost_system
        EXPORT mojiTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )

  install(
      DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

  install(
      DIRECTORY ${moji_BOOST_SOURCE_DIR}/boost 
      DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

  install(
        EXPORT mojiTargets
        NAMESPACE moji::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/moji
    )

  configure_package_config_file(
        cmake/mojiConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/mojiConfig.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/moji
    )

  write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/mojiConfigVersion.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
    )

  install(
        FILES
            ${CMAKE_CURRENT_BINARY_DIR}/mojiConfig.cmake
            ${CMAKE_CURRENT_BINARY_DIR}/mojiConfigVersion.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/moji
    )
endif()
