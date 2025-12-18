cmake_minimum_required(VERSION 3.12)
project(poetry_search VERSION 1.0.0 LANGUAGES C CXX)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

add_subdirectory(./third-party/pybind11)

find_package(Python REQUIRED COMPONENTS Interpreter Development.Module)
find_package(OpenMP REQUIRED)
find_package(LLVM REQUIRED CONFIG)

include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/third-party)

file(GLOB SOURCES "${CMAKE_SOURCE_DIR}/src/*.cpp")

llvm_map_components_to_libnames(LLVM_LIBS
    Core
    Support
    IRReader
    ExecutionEngine
    MCJIT
    native
)

pybind11_add_module(poetry_search ${SOURCES})
target_link_libraries(poetry_search PRIVATE OpenMP::OpenMP_CXX ${LLVM_LIBS})