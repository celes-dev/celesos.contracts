cmake_minimum_required(VERSION 3.5)
project(celesos.bios VERSION 1.0.0)

find_package(celesos.cdt)

add_contract(celesos.bios celesos.bios ${CMAKE_CURRENT_SOURCE_DIR}/src/celesos.bios.cpp)
target_include_directories(celesos.bios.wasm
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(celesos.bios.wasm
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
