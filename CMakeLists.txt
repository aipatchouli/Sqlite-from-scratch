cmake_minimum_required(VERSION 3.26)
project(Sqlite_from_scratch)

set(CMAKE_CXX_STANDARD 20)

include_directories(src)

add_executable(Sqlite_from_scratch
        src/utils.hpp
        database.cpp)
