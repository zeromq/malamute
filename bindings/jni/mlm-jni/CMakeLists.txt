################################################################################
#  THIS FILE IS 100% GENERATED BY ZPROJECT; DO NOT EDIT EXCEPT EXPERIMENTALLY  #
#  Read the zproject/README.md for information about making permanent changes. #
################################################################################
cmake_minimum_required (VERSION 2.8)

project (mlmjni CXX)
enable_language (C)

# Search for Find*.cmake files in the following locations
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/../../..")

########################################################################
# JNI dependency
########################################################################
find_package (JNI REQUIRED)
include_directories (${JAVA_INCLUDE_PATH} ${JAVA_INCLUDE_PATH2} src/native/include)

########################################################################
# LIBZMQ dependency
########################################################################
find_package(libzmq REQUIRED)
IF (LIBZMQ_FOUND)
    include_directories(${libzmq_INCLUDE_DIRS})
    list(APPEND MORE_LIBRARIES ${libzmq_LIBRARIES})
ELSE (LIBZMQ_FOUND)
    message( FATAL_ERROR "libzmq not found." )
ENDIF (LIBZMQ_FOUND)

########################################################################
# CZMQ dependency
########################################################################
find_package(czmq REQUIRED)
IF (CZMQ_FOUND)
    include_directories(${czmq_INCLUDE_DIRS})
    list(APPEND MORE_LIBRARIES ${czmq_LIBRARIES})
ELSE (CZMQ_FOUND)
    message( FATAL_ERROR "czmq not found." )
ENDIF (CZMQ_FOUND)

########################################################################
# MLM dependency
########################################################################
find_package(mlm REQUIRED)
IF (MLM_FOUND)
    include_directories(${mlm_INCLUDE_DIRS})
    list(APPEND MORE_LIBRARIES ${mlm_LIBRARIES})
ELSE (MLM_FOUND)
    message( FATAL_ERROR "mlm not found." )
ENDIF (MLM_FOUND)

set (mlmjni_sources
    src/main/c/org_zeromq_mlm_MlmProto.c
    src/main/c/org_zeromq_mlm_MlmClient.c
)

add_library (mlmjni SHARED ${mlmjni_sources})
add_definitions (-DMLM_BUILD_DRAFT_API)

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic -O2")
set (CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build)
set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build)
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build)

target_link_libraries (mlmjni ${MORE_LIBRARIES})
