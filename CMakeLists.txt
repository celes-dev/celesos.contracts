cmake_minimum_required(VERSION 3.5)
project(eosio_contracts VERSION 1.5.2)

set(CELESOS_CDT_VERSION_MIN "1.4")
set(CELESOS_CDT_VERSION_SOFT_MAX "1.5")
#set(EOSIO_CDT_VERSION_HARD_MAX "")

find_package(celesos.cdt)

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


add_subdirectory(celes.token)
add_subdirectory(celes.unregd)
add_subdirectory(celesos.bios)
add_subdirectory(celesos.msig)
add_subdirectory(celesos.wrap)
add_subdirectory(celesos.system)
add_subdirectory(celesos.test)

if (APPLE)
   set(OPENSSL_ROOT "/usr/local/opt/openssl")
elseif (UNIX)
   set(OPENSSL_ROOT "/usr/include/openssl")
endif()
set(SECP256K1_ROOT "/usr/local")

#include(UnitTestsExternalProject.txt)
