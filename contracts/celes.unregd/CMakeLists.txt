cmake_minimum_required(VERSION 3.5)
project(celesos.unregd VERSION 1.0.0)

find_package(celesos.cdt)

add_contract(celes.unregd celes.unregd 
   ${CMAKE_CURRENT_SOURCE_DIR}/src/celes.unregd.cpp
)
   
#target_include_directories(celes.unregd.wasm
#   PUBLIC
#   ${CMAKE_CURRENT_SOURCE_DIR}/include
#)

set_target_properties(celes.unregd
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
