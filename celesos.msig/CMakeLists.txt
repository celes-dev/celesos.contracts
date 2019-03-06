cmake_minimum_required(VERSION 3.5)
project(celesos.msig VERSION 1.0.0)

find_package(celesos.cdt)

add_contract(celesos.msig celesos.msig ${CMAKE_CURRENT_SOURCE_DIR}/src/celesos.msig.cpp)
target_include_directories(celesos.msig.wasm
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(celesos.msig.wasm
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
