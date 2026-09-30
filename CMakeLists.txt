cmake_minimum_required(VERSION 3.10)
project(TrigVisualizer VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra)
endif()

add_executable(trig_visualizer src/main.cpp)

enable_testing()
add_executable(trig_tests tests/tests.cpp)
add_test(NAME unit_tests COMMAND trig_tests)
