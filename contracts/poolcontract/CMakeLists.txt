add_contract(poolcontract poolcontract ${CMAKE_CURRENT_SOURCE_DIR}/src/poolcontract.cpp)

target_include_directories(poolcontract
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(poolcontract
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

# configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/ricardian/poolcontract.contracts.md.in ${CMAKE_CURRENT_BINARY_DIR}/ricardian/poolcontract.contracts.md @ONLY )

# target_compile_options( poolcontract PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/ricardian -R${CMAKE_CURRENT_BINARY_DIR}/ricardian )