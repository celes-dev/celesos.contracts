add_contract(celesos.msig celesos.msig ${CMAKE_CURRENT_SOURCE_DIR}/src/celesos.msig.cpp)

target_include_directories(celesos.msig
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(celesos.msig
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/ricardian/celesos.msig.contracts.md.in ${CMAKE_CURRENT_BINARY_DIR}/ricardian/celesos.msig.contracts.md @ONLY )

target_compile_options( celesos.msig PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/ricardian -R${CMAKE_CURRENT_BINARY_DIR}/ricardian )
