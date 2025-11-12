# CMakeList.txt : CMake project for core_as, include source and define
# project specific logic here.
#
add_executable(test_json test_json.cpp)
target_link_libraries(test_json simjson::simjson GTest::gtest_main)

#[[
# Пример создания экзешника, если simjson и simstr экспортируются из dll.
# Так как мы подключаем только simjson_dll, и входящие в неё либы не видны
# нужно вручную задать каталоги include и стандарт C++20
if (SIMJSON_IN_SHARED)
    target_link_libraries(test_json simjson_dll GTest::gtest_main)
    get_target_property(SIMJSON_INCLUDES simjson::simjson INCLUDE_DIRECTORIES)
    get_target_property(SIMSTR_INCLUDES simstr::simstr INCLUDE_DIRECTORIES)
    target_include_directories(test_json PRIVATE ${SIMJSON_INCLUDES} ${SIMSTR_INCLUDES})
    target_compile_features(test_json PUBLIC cxx_std_20)
endif()
#]]

add_test(NAME test_json COMMAND test_json WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

if (EMSCRIPTEN)
    set_target_properties (test_json PROPERTIES SUFFIX .html)
endif(EMSCRIPTEN)
