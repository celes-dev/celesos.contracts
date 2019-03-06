cmake_minimum_required(VERSION 3.5)
project(celesos.token VERSION 1.0.0)

find_package(celesos.cdt)

add_contract(celes.token celes.token ${CMAKE_CURRENT_SOURCE_DIR}/src/celes.token.cpp)
target_include_directories(celes.token.wasm
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(celes.token.wasm
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
