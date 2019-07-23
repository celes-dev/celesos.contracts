add_contract(celes.token celes.token ${CMAKE_CURRENT_SOURCE_DIR}/src/celes.token.cpp)

target_include_directories(celes.token
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(celes.token
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/ricardian/celes.token.contracts.md.in ${CMAKE_CURRENT_BINARY_DIR}/ricardian/celes.token.contracts.md @ONLY )

target_compile_options( celes.token PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/ricardian -R${CMAKE_CURRENT_BINARY_DIR}/ricardian )
