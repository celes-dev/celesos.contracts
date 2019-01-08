add_library(eth.crypto STATIC
   ${CMAKE_CURRENT_SOURCE_DIR}/src/sha3/byte_order.c
   ${CMAKE_CURRENT_SOURCE_DIR}/src/sha3/sha3.c
   ${CMAKE_CURRENT_SOURCE_DIR}/src/ecc/uECC.c)
   
target_compile_definitions(eth.crypto PRIVATE 
   uECC_SUPPORTS_secp160r1=0
   uECC_SUPPORTS_secp192r1=0
   uECC_SUPPORTS_secp224r1=0
   uECC_SUPPORTS_secp256r1=0
   uECC_SUPPORTS_secp256k1=1
   uECC_SUPPORT_COMPRESSED_POINT=1)

target_include_directories(eth.crypto
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)