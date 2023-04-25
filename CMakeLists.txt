cmake_minimum_required(VERSION 3.26)
project(SQlite_from_Scratch)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_COMPILE_FEATURES -std=c++2b -O2 )

# 指定输出路径
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)

include_directories(src)


add_executable(database
                src/RowStruction.hpp
                src/RowandPage.hpp
                src/TypeEnum.hpp
                src/utils.hpp
                database.cpp)
