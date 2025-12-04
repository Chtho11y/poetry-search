cmake_minimum_required(VERSION 3.12)
project(poetry_search VERSION 1.0.0 LANGUAGES C CXX)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

add_subdirectory(./third-party/pybind11)

find_package(Python REQUIRED COMPONENTS Interpreter Development.Module)

include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/third-party)

file(GLOB SOURCES "${CMAKE_SOURCE_DIR}/src/*.cpp")

pybind11_add_module(poetry_search ${SOURCES})