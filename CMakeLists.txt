cmake_minimum_required(VERSION 3.20)
include(cmake/prelude.cmake)
include(CMakeDependentOption)
include(FetchContent)

project(
    simjson
    VERSION 1.0.0
    DESCRIPTION "Very simple json library"
    HOMEPAGE_URL "https://github.com/orefkov/simjson"
    LANGUAGES CXX
)

include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)

if(${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
    set(MSVC_COMPILER ON)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8 /Zc:strictStrings")
elseif(${CMAKE_CXX_COMPILER_ID} STREQUAL Clang)
    set(CLANG_COMPILER ON)
endif()

option(SIMJSON_BUILD_TESTS "Построить тесты" ON)
option(SIMJSON_BENCHMARKS "Построить замеры производительности" OFF)
option(SIMJSON_SHARED "Функции simjson должны экспортироваться или импортироваться" OFF)

add_library(simjson_simjson
    src/json.cpp
)
add_library(simjson::simjson ALIAS simjson_simjson)

target_include_directories(
    simjson_simjson ${warning_guard}
    PUBLIC
    "\$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
)

target_compile_features(simjson_simjson PUBLIC cxx_std_20)

# Для MSVC подключаем natvis файл для красивой отладки
if(${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
    target_link_options(simjson_simjson PUBLIC "/natvis:${CMAKE_CURRENT_SOURCE_DIR}/for_debug/simjson.natvis")
endif()

set_target_properties(
    simjson_simjson PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN YES
    VERSION "${PROJECT_VERSION}"
    SOVERSION "${PROJECT_VERSION_MAJOR}"
    EXPORT_NAME simjson
    OUTPUT_NAME simjson
)

function(Simstr)
    set(SIMSTR_BUILD_TESTS OFF)
    set(SIMSTR_BENCHMARKS OFF)
    if(SIMJSON_SHARED)
        set(SIMSTR_SHARED On)
    endif()
    FetchContent_Declare(
        simstr
        GIT_REPOSITORY https://github.com/orefkov/simstr.git
        GIT_SHALLOW TRUE
        GIT_TAG b8296f5
        FIND_PACKAGE_ARGS
    )
    FetchContent_MakeAvailable(simstr)
endfunction()

Simstr()

target_link_libraries(simjson_simjson PUBLIC simstr::simstr)

if(SIMJSON_SHARED)
    add_compile_definitions(SIMJSON_SHARED)
    target_compile_definitions(simjson_simjson PRIVATE SIMJSON_EXPORT)
endif(SIMJSON_SHARED)

if(SIMJSON_BUILD_TESTS)
    enable_testing()
    set(BUILD_TESTS ON)
    # Load and build GTest
    FetchContent_Declare(
        googletest
        # Specify the commit you depend on and update it regularly.
        URL https://github.com/google/googletest/archive/refs/tags/release-1.12.1.zip
    )

    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt FALSE CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
    add_subdirectory(tests)
endif()

function(GBencmark)
    # Load and build Google benchmarks
    include(FetchContent)
    FetchContent_Declare(
        googlebench
        # Specify the commit you depend on and update it regularly.
        URL https://github.com/google/benchmark/archive/refs/tags/v1.7.0.zip
    )
    set(CMAKE_CXX_STANDARD 20)
    set(BENCHMARK_ENABLE_TESTING OFF)
    set(BENCHMARK_ENABLE_LTO OFF)
    set(BENCHMARK_ENABLE_INSTALL OFF)
    set(BENCHMARK_INSTALL_DOCS OFF)
    set(BENCHMARK_DOWNLOAD_DEPENDENCIES ON)
    set(BENCHMARK_ENABLE_GTEST_TESTS OFF)
    add_compile_definitions(BENCHMARK_STATIC_DEFINE)
    FetchContent_MakeAvailable(googlebench)
endfunction()

if(SIMJSON_BENCHMARKS) # AND CMAKE_BUILD_TYPE STREQUAL Release)
    set(BUILD_BENCHMARKS ON)
    GBencmark()
    add_subdirectory(bench)
endif()

# ---- Install rules ----

if(NOT CMAKE_SKIP_INSTALL_RULES)
    include(cmake/install-rules.cmake)
endif()

# ---- Developer mode ----

if(NOT simjsons_DEVELOPER_MODE)
    return()
elseif(NOT PROJECT_IS_TOP_LEVEL)
    message(
        AUTHOR_WARNING
        "Developer mode is intended for developers of simjson"
    )
endif()

include(cmake/dev-mode.cmake)
