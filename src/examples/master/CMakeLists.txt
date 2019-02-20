cmake_minimum_required (VERSION 3.0)

project (master C)

include(${CMAKE_CURRENT_LIST_DIR}/../../core/dawn.cmake)

include_directories (${DAWN_SOURCES_DIR})


ADD_DEFINITIONS(
-O3 -g -W -Wall
 -Wunused-variable -Wunused-parameter -Wunused-function -Wunused
 -Wno-deprecated -Wwrite-strings
 -D__WUR= -D_REENTRANT -D_FILE_OFFSET_BITS=64 -DTIXML_USE_STL
)

SET(SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/master.c
    )

add_executable(${PROJECT_NAME} ${SOURCES} ${DAWN_SOURCES})
