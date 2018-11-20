cmake_minimum_required(VERSION 3.5)
project(celes.wrap VERSION 1.0.0)

find_package(eosio.cdt)

add_contract(celes.wrap celes.wrap ${CMAKE_CURRENT_SOURCE_DIR}/src/celes.wrap.cpp)
target_include_directories(celes.wrap.wasm
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(celes.wrap.wasm
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
