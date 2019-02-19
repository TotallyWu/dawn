cmake_minimum_required (VERSION 3.0)

project (dawn_core C)

#INCLUDE_DIRECTORIES(../)
#INCLUDE_DIRECTORIES(../common)

SET(SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/dawn.c
    )

add_executable(${PROJECT_NAME} ${SOURCES})
