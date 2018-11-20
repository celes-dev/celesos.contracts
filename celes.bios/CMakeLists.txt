cmake_minimum_required(VERSION 3.5)
project(celes.bios VERSION 1.0.0)

find_package(eosio.cdt)

add_contract(celes.bios celes.bios ${CMAKE_CURRENT_SOURCE_DIR}/src/celes.bios.cpp)
target_include_directories(celes.bios.wasm
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(celes.bios.wasm
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
