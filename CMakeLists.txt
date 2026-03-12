cmake_minimum_required(VERSION 3.15)
project(minilang LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options()

find_package(LLVM REQUIRED CONFIG)

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

include_directories(include)

add_executable(minilang
    src/grammar.cpp
    src/main.cpp
    src/parser.cpp
    src/lexer.cpp
    src/token.cpp
    src/symbol.cpp
    src/semantic.cpp
    src/irgen.cpp
)

llvm_map_components_to_libnames(llvm_libs irreader)

target_include_directories(minilang PRIVATE include)
target_link_libraries(minilang PRIVATE ${llvm_libs})
