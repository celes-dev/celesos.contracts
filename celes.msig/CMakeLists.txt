cmake_minimum_required(VERSION 3.5)
project(celes.msig VERSION 1.0.0)

find_package(eosio.cdt)

add_contract(celes.msig celes.msig ${CMAKE_CURRENT_SOURCE_DIR}/src/celes.msig.cpp)
target_include_directories(celes.msig.wasm
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(celes.msig.wasm
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
