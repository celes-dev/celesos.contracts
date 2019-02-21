cmake_minimum_required(VERSION 3.5)
project(celesos.wrap VERSION 0.9.0)

find_package(celesos.cdt)

add_contract(celesos.wrap celesos.wrap ${CMAKE_CURRENT_SOURCE_DIR}/src/celesos.wrap.cpp)
target_include_directories(celesos.wrap.wasm
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(celesos.wrap.wasm
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
