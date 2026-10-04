cmake_minimum_required(VERSION 3.16)
project(charcoal1 VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(charcoal1_lib
    src/lexer.cpp
    src/parser.cpp
    src/compiler.cpp
    src/vm.cpp
    src/gc.cpp
)
target_include_directories(charcoal1_lib PUBLIC src)

add_executable(charcoal1_demo src/main.cpp)
target_link_libraries(charcoal1_demo PRIVATE charcoal1_lib)

enable_testing()
add_executable(charcoal1_tests tests/test_lexer.cpp)
target_link_libraries(charcoal1_tests PRIVATE charcoal1_lib)
add_test(NAME lexer_tests COMMAND charcoal1_tests)
