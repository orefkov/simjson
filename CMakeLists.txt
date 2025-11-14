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
option(SIMJSON_IN_SHARED "Функции simjson будут экспортироваться или импортироваться" Off)

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

if (NOT CMAKE_CXX_SIMULATE_ID)
    set (CMAKE_CXX_SIMULATE_ID "None")
endif()
# Для MSVC подключаем natvis файл для красивой отладки
if(${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC OR ${CMAKE_CXX_SIMULATE_ID} STREQUAL MSVC)
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

function(add_simstr)
    set(SIMSTR_BUILD_TESTS OFF)
    set(SIMSTR_BENCHMARKS OFF)
    if(SIMJSON_IN_SHARED)
        set(SIMSTR_IN_SHARED On)
    endif()
    FetchContent_Declare(
        simstr
        GIT_REPOSITORY https://github.com/orefkov/simstr.git
        GIT_SHALLOW TRUE
        GIT_TAG 4ddb1ae
        FIND_PACKAGE_ARGS NAMES simstr
    )
    FetchContent_MakeAvailable(simstr)
endfunction()

add_simstr()

target_link_libraries(simjson_simjson PUBLIC simstr::simstr)

if(SIMJSON_IN_SHARED)
    # Всем объявляем, что мы будем в shared библиотеке
    add_compile_definitions(SIMJSON_IN_SHARED)
    # Себе объявим, что мы должны экспортировать функции
    target_compile_definitions(simjson_simjson PRIVATE SIMJSON_EXPORT)

    # Пример построения shared библиотеки, которая включит в себя simjson и simstr.
    # Как при этом создать экзешник, пользующийся этой dll - пример в tests/CMakeLists.txt
    # Нельзя объявить библиотеку без исходников, добавим просто .h файл
    add_library(simjson_dll SHARED include/simjson/json.h)
    # simstr нельзя просто так добавить как whole_archive, CMake при сборке simjson уже пометил её как DEFAULT
    target_link_libraries(simjson_dll PRIVATE
        "$<LINK_LIBRARY:WHOLE_ARCHIVE,simjson::simjson>"
        simstr_simstr
    )
    # Но можно для нужного таргета переопределить её как whole_archive
    set_property(TARGET simjson_dll PROPERTY LINK_LIBRARY_OVERRIDE "WHOLE_ARCHIVE,simstr_simstr")
endif(SIMJSON_IN_SHARED)

if(SIMJSON_BUILD_TESTS)
    enable_testing()
    set(BUILD_TESTS ON)
    # Load and build GTest
    FetchContent_Declare(
        googletest
        # Specify the commit you depend on and update it regularly.
        URL https://github.com/google/googletest/archive/refs/tags/release-1.12.1.zip
        FIND_PACKAGE_ARGS NAMES GTest
    )
    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt FALSE CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
    add_subdirectory(tests)
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
