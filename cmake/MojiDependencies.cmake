option(moji_FETCH_BOOST "Build Boost from source instead of using an installed package" ON)

set(moji_BOOST_SOURCE_DIR "" CACHE PATH "Path to a Boost 1.85+ release source tree. Used only when moji_FETCH_BOOST is ON.")

function(moji_fetch_boost)
    if(moji_BOOST_SOURCE_DIR)
        get_filename_component(boost_source_dir "${moji_BOOST_SOURCE_DIR}" ABSOLUTE)
        set(moji_BOOST_SOURCE_DIR "${boost_source_dir}" PARENT_SCOPE)
        return()
    endif()

    include(FetchContent)
    message(STATUS "moji: Fetching Boost 1.85.0...")

    if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.24)
        FetchContent_Declare(
            boost_all
            URL https://archives.boost.io/release/1.85.0/source/boost_1_85_0.tar.gz
            SOURCE_SUBDIR cmake/moji-no-project
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        )
    else()
        FetchContent_Declare(
            boost_all
            URL https://archives.boost.io/release/1.85.0/source/boost_1_85_0.tar.gz
            SOURCE_SUBDIR cmake/moji-no-project
        )
    endif()

    FetchContent_MakeAvailable(boost_all)
    FetchContent_GetProperties(boost_all)

    set(moji_BOOST_SOURCE_DIR "${boost_all_SOURCE_DIR}" PARENT_SCOPE)
endfunction()

function(moji_setup_boost)
    set(moji_BUNDLED_BOOST_TARGETS "" PARENT_SCOPE)

    if(NOT moji_FETCH_BOOST)
        message(STATUS "moji: Using installed Boost (package/toolchain search paths)")
        find_package(Boost 1.85 CONFIG REQUIRED COMPONENTS headers system json url)
        return()
    endif()

    # A parent that already provides Boost must select the package path, rather
    # than combine its Boost headers/libraries with our bundled release.
    foreach(component headers system json url)
        if(TARGET Boost::${component})
            message(FATAL_ERROR
                "Boost::${component} already exists. Set moji_FETCH_BOOST=OFF to use the parent's Boost package.")
        endif()
    endforeach()

    moji_fetch_boost()
    foreach(required_file
        boost/version.hpp boost/asio.hpp boost/beast.hpp boost/json.hpp boost/url.hpp
        libs/json/src/src.cpp libs/url/CMakeLists.txt LICENSE_1_0.txt)
        if(NOT EXISTS "${moji_BOOST_SOURCE_DIR}/${required_file}")
            message(FATAL_ERROR
                "Incomplete Boost release source tree: ${moji_BOOST_SOURCE_DIR}/${required_file} is missing.")
        endif()
    endforeach()

    file(STRINGS "${moji_BOOST_SOURCE_DIR}/boost/version.hpp" boost_version_line
        REGEX "^#define BOOST_VERSION [0-9]+$")
    string(REGEX REPLACE ".*BOOST_VERSION ([0-9]+).*" "\\1" boost_version "${boost_version_line}")
    if(NOT boost_version MATCHES "^[0-9]+$" OR boost_version LESS 108500)
        message(FATAL_ERROR "moji requires a Boost 1.85 or newer release source tree.")
    endif()
    message(STATUS "moji: Building bundled Boost from ${moji_BOOST_SOURCE_DIR}")

    # Release archives have a unified boost/ include tree, but no top-level
    # CMake project. Build just JSON and URL; System is header-only since 1.69.
    add_library(moji_boost_headers INTERFACE)
    add_library(Boost::headers ALIAS moji_boost_headers)
    target_include_directories(moji_boost_headers SYSTEM INTERFACE
        $<BUILD_INTERFACE:${moji_BOOST_SOURCE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

    add_library(moji_boost_system INTERFACE)
    add_library(Boost::system ALIAS moji_boost_system)
    target_link_libraries(moji_boost_system INTERFACE Boost::headers)

    add_library(moji_boost_json STATIC "${moji_BOOST_SOURCE_DIR}/libs/json/src/src.cpp")
    add_library(Boost::json ALIAS moji_boost_json)
    target_link_libraries(moji_boost_json PUBLIC Boost::system)
    target_compile_features(moji_boost_json PUBLIC cxx_std_11)
    target_compile_definitions(moji_boost_json PUBLIC BOOST_JSON_NO_LIB=1 BOOST_JSON_STATIC_LINK=1)

    # This is the same source selection used by Boost.URL's own CMake project.
    file(GLOB_RECURSE boost_url_sources CONFIGURE_DEPENDS "${moji_BOOST_SOURCE_DIR}/libs/url/src/*.cpp")
    if(NOT boost_url_sources)
        message(FATAL_ERROR "No Boost.URL sources found in ${moji_BOOST_SOURCE_DIR}/libs/url/src.")
    endif()
    add_library(moji_boost_url STATIC ${boost_url_sources})
    add_library(Boost::url ALIAS moji_boost_url)
    target_link_libraries(moji_boost_url PUBLIC Boost::system)
    target_compile_features(moji_boost_url PUBLIC cxx_std_11)
    target_compile_definitions(moji_boost_url
        PUBLIC BOOST_URL_NO_LIB=1 BOOST_URL_STATIC_LINK=1
        PRIVATE BOOST_URL_SOURCE)

    set(moji_BOOST_SOURCE_DIR "${moji_BOOST_SOURCE_DIR}" PARENT_SCOPE)
    set(moji_BUNDLED_BOOST_TARGETS
        moji_boost_headers moji_boost_system moji_boost_json moji_boost_url
        PARENT_SCOPE)
    foreach(component headers system json url)
        set_target_properties(moji_boost_${component} PROPERTIES EXPORT_NAME boost_${component})
    endforeach()
endfunction()
