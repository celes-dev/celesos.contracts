add_contract(celesos.test celesos.test ${CMAKE_CURRENT_SOURCE_DIR}/src/celesos.test.cpp)

target_include_directories(celesos.test
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(celesos.test
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/ricardian/celesos.test.contracts.md.in ${CMAKE_CURRENT_BINARY_DIR}/ricardian/celesos.test.contracts.md @ONLY )

target_compile_options( celesos.test PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/ricardian -R${CMAKE_CURRENT_BINARY_DIR}/ricardian )
