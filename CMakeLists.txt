cmake_minimum_required(VERSION 3.16)
project(TranslatorLexer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(TranslatorLexer
        main.cpp
        lexer.cpp
        trie.cpp
        parser.cpp
        semanter.cpp
        poliz.cpp
        vm.cpp
        vm.hpp
        )