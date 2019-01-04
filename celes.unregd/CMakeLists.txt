cmake_minimum_required(VERSION 3.5)
project(eosio.unregd VERSION 1.0.0)

find_package(celesos.cdt)

add_contract(celes.unregd celes.unregd 
   ${CMAKE_CURRENT_SOURCE_DIR}/src/sha3/byte_order.c
   ${CMAKE_CURRENT_SOURCE_DIR}/src/sha3/sha3.c
   ${CMAKE_CURRENT_SOURCE_DIR}/src/ecc/uECC.c
   ${CMAKE_CURRENT_SOURCE_DIR}/src/exchange_state.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/src/eosio.unregd.cpp)
   
target_compile_definitions(celes.unregd.wasm PRIVATE 
   uECC_SUPPORTS_secp160r1=0
   uECC_SUPPORTS_secp192r1=0
   uECC_SUPPORTS_secp224r1=0
   uECC_SUPPORTS_secp256r1=0
   uECC_SUPPORTS_secp256k1=1
   uECC_SUPPORT_COMPRESSED_POINT=1)

target_include_directories(celes.unregd.wasm
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(celes.unregd.wasm
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
