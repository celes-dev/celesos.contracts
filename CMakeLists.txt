cmake_minimum_required(VERSION 3.5)

project(celesos_contracts)

set(VERSION_MAJOR 1)
set(VERSION_MINOR 0)
set(VERSION_PATCH 0)
#set(VERSION_SUFFIX develop)

if (VERSION_SUFFIX)
   set(VERSION_FULL "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}-${VERSION_SUFFIX}")
else()
   set(VERSION_FULL "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")
endif()

include(ExternalProject)

find_package(celesos.cdt)

message(STATUS "Building celesos.contracts v${VERSION_FULL}")

set(CELESOS_CDT_VERSION_MIN "1.0")
set(CELESOS_CDT_VERSION_SOFT_MAX "1.0")
#set(CELESOS_CDT_VERSION_HARD_MAX "")

### Check the version of celesos.cdt
set(VERSION_MATCH_ERROR_MSG "")
CELESOS_CHECK_VERSION(VERSION_OUTPUT "${CELESOS_CDT_VERSION}"
                                   "${CELESOS_CDT_VERSION_MIN}"
                                   "${CELESOS_CDT_VERSION_SOFT_MAX}"
                                   "${CELESOS_CDT_VERSION_HARD_MAX}"
                                   VERSION_MATCH_ERROR_MSG)
if(VERSION_OUTPUT STREQUAL "MATCH")
   message(STATUS "Using celesos.cdt version ${CELESOS_CDT_VERSION}")
elseif(VERSION_OUTPUT STREQUAL "WARN")
   message(WARNING "Using celesos.cdt version ${CELESOS_CDT_VERSION} even though it exceeds the maximum supported version of ${CELESOS_CDT_VERSION_SOFT_MAX}; continuing with configuration, however build may fail.\nIt is recommended to use celesos.cdt version ${CELESOS_CDT_VERSION_SOFT_MAX}.x")
else() # INVALID OR MISMATCH
   message(FATAL_ERROR "Found celesos.cdt version ${CELESOS_CDT_VERSION} but it does not satisfy version requirements: ${VERSION_MATCH_ERROR_MSG}\nPlease use celesos.cdt version ${CELESOS_CDT_VERSION_SOFT_MAX}.x")
endif(VERSION_OUTPUT STREQUAL "MATCH")

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
   set(TEST_BUILD_TYPE "Debug")
   set(CMAKE_BUILD_TYPE "Release")
else()
   set(TEST_BUILD_TYPE ${CMAKE_BUILD_TYPE})
endif()

ExternalProject_Add(
   contracts_project
   SOURCE_DIR ${CMAKE_SOURCE_DIR}/contracts
   BINARY_DIR ${CMAKE_BINARY_DIR}/contracts
   CMAKE_ARGS -DCMAKE_TOOLCHAIN_FILE=${CELESOS_CDT_ROOT}/lib/cmake/celesos.cdt/EosioWasmToolchain.cmake
   UPDATE_COMMAND ""
   PATCH_COMMAND ""
   TEST_COMMAND ""
   INSTALL_COMMAND ""
   BUILD_ALWAYS 1
)

# if (APPLE)
#    set(OPENSSL_ROOT "/usr/local/opt/openssl")
# elseif (UNIX)
#    set(OPENSSL_ROOT "/usr/include/openssl")
# endif()
# set(SECP256K1_ROOT "/usr/local")

# if (APPLE)
#    set(OPENSSL_ROOT "/usr/local/opt/openssl")
# elseif (UNIX)
#    set(OPENSSL_ROOT "/usr/include/openssl")
# endif()
# set(SECP256K1_ROOT "/usr/local")

# string(REPLACE ";" "|" TEST_FRAMEWORK_PATH "${CMAKE_FRAMEWORK_PATH}")
# string(REPLACE ";" "|" TEST_MODULE_PATH "${CMAKE_MODULE_PATH}")

# ExternalProject_Add(
#   contracts_unit_tests
#   LIST_SEPARATOR | # Use the alternate list separator
#   CMAKE_ARGS -DCMAKE_BUILD_TYPE=${TEST_BUILD_TYPE} -DCMAKE_FRAMEWORK_PATH=${TEST_FRAMEWORK_PATH} -DCMAKE_MODULE_PATH=${TEST_MODULE_PATH} -DCELESOS_ROOT=${CELESOS_ROOT} -DLLVM_DIR=${LLVM_DIR}
#   SOURCE_DIR ${CMAKE_SOURCE_DIR}/tests
#   BINARY_DIR ${CMAKE_BINARY_DIR}/tests
#   BUILD_ALWAYS 1
#   TEST_COMMAND   ""
#   INSTALL_COMMAND ""
# )