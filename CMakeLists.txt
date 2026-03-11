cmake_minimum_required(VERSION 3.15)
project(minilang LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(-Wall -Wextra)


include_directories(include)

add_executable(test_grammar
    src/grammar.cpp
    src/main.cpp
    src/parser.cpp
    src/lexer.cpp
    src/token.cpp
)

target_include_directories(test_grammar PRIVATE include)
