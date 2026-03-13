cmake_minimum_required(VERSION 3.15)
project(minilang LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(LLVM REQUIRED CONFIG)

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

include_directories(include)


llvm_map_components_to_libnames(LLVM_LIBS
    ARM

    AArch64CodeGen
    AArch64AsmParser
    AArch64Desc
    AArch64Info
    AArch64Disassembler
    AArch64Utils
)

add_executable(minilang
    src/grammar.cpp
    src/main.cpp
    src/parser.cpp
    src/lexer.cpp
    src/token.cpp
    src/symbol.cpp
    src/semantic.cpp
    src/irgen.cpp
    src/codegen.cpp
)

target_include_directories(minilang PRIVATE include)
target_link_libraries(minilang PRIVATE ${LLVM_LIBS})
