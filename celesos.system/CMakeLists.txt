cmake_minimum_required(VERSION 3.5)
project(hello_example VERSION 1.0.0)

find_package(celesos.cdt)


add_contract(celesos.system celesos.system ${CMAKE_CURRENT_SOURCE_DIR}/src/celesos.system.cpp)
#add_executable(celesos.system.wasm ${CMAKE_CURRENT_SOURCE_DIR}/src/celesos.system.cpp)
target_include_directories(celesos.system.wasm
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../celes.token/include)

set_target_properties(celesos.system.wasm
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
