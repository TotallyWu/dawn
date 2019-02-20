cmake_minimum_required (VERSION 3.0)

project (slave C)

INCLUDE_DIRECTORIES(../../core)

SET(SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/salve.c
    )

add_executable(${PROJECT_NAME} ${SOURCES})
