cmake_minimum_required(VERSION 3.5)
project(celesos.wrap VERSION 1.0.0)

find_package(eosio.cdt)

add_contract(celesos.test celesos.test ${CMAKE_CURRENT_SOURCE_DIR}/src/celesos.test.cpp)
target_include_directories(celesos.test.wasm
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(celesos.test.wasm
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
