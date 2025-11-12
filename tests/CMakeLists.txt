# CMakeList.txt : CMake project for core_as, include source and define
# project specific logic here.
#
add_executable(test_json test_json.cpp)
target_link_libraries(test_json simjson_simjson gtest_main)
add_test(NAME test_json COMMAND test_json)

if (EMSCRIPTEN)
    set_target_properties (test_json PROPERTIES SUFFIX .html)
endif(EMSCRIPTEN)
