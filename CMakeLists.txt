cmake_minimum_required(VERSION 3.20)
project(moji VERSION 1.0.0 LANGUAGES CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(cmake/MojiDependencies.cmake)

if(PROJECT_IS_TOP_LEVEL)
  set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
  set(moji_DEFAULT_INSTALL ON)
else()
  set(moji_DEFAULT_INSTALL OFF)
endif()

option(
  moji_INSTALL "Generate install rules for moji" ${moji_DEFAULT_INSTALL})

moji_setup_boost()
find_package(Threads REQUIRED)

set(moji_SOURCES
    src/app.cpp
    src/config.cpp
    src/http_session.cpp
    src/json.cpp
    src/request.cpp
    src/dispatcher.cpp
    src/response.cpp
    src/router.cpp
    src/server.cpp
    src/listener.cpp
    src/engine_pool.cpp
    src/signal_handler.cpp
    src/url.cpp
)

add_library(moji_core STATIC ${moji_SOURCES})
add_library(moji::core ALIAS moji_core)
set_target_properties(moji_core PROPERTIES EXPORT_NAME core)
target_compile_features(moji_core PUBLIC cxx_std_23)

target_include_directories(moji_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(moji_core
    PUBLIC Boost::headers Boost::system Threads::Threads
    PRIVATE Boost::json Boost::url
)

if(moji_INSTALL)
  install(
        TARGETS moji_core ${moji_BUNDLED_BOOST_TARGETS}
        EXPORT mojiTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )

  install(
      DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

  if(moji_FETCH_BOOST)
    install(
        DIRECTORY "${moji_BOOST_SOURCE_DIR}/boost"
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
    install(
        FILES "${moji_BOOST_SOURCE_DIR}/LICENSE_1_0.txt"
        DESTINATION ${CMAKE_INSTALL_DATADIR}/licenses/moji/boost)
  endif()

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
