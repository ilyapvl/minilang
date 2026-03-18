cmake_minimum_required(VERSION 3.15)
project(minilang LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_TESTS "Build tests" ON)

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

add_library(minilang_lib
    src/grammar.cpp
    src/parser.cpp
    src/lexer.cpp
    src/token.cpp
    src/symbol.cpp
    src/semantic.cpp
    src/irgen.cpp
    src/codegen.cpp
)
target_include_directories(minilang_lib PUBLIC include)

add_executable(minilang src/main.cpp)
target_link_libraries(minilang PRIVATE minilang_lib ${LLVM_LIBS})
target_include_directories(minilang PRIVATE include)




if(BUILD_TESTS)

    include(FetchContent)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
    )
    FetchContent_MakeAvailable(googletest)

    file(GLOB_RECURSE TEST_SOURCES "test/*.cpp")

    if(NOT TEST_SOURCES)
        message(WARNING "No test sources found in test/ directory. Skipping test build.")
    else()
        add_executable(minilang_tests ${TEST_SOURCES})
        target_link_libraries(minilang_tests
            minilang_lib
            ${LLVM_LIBS}
            GTest::gtest_main
        )
        target_include_directories(minilang_tests PRIVATE include)

        include(GoogleTest)
        gtest_discover_tests(minilang_tests)
    endif()
endif()
