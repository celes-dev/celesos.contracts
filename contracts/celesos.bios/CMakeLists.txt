add_contract(celesos.bios celesos.bios ${CMAKE_CURRENT_SOURCE_DIR}/src/celesos.bios.cpp)

target_include_directories(celesos.bios
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(celesos.bios
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/ricardian/celesos.bios.contracts.md.in ${CMAKE_CURRENT_BINARY_DIR}/ricardian/celesos.bios.contracts.md @ONLY )

target_compile_options( celesos.bios PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/ricardian -R${CMAKE_CURRENT_BINARY_DIR}/ricardian )
