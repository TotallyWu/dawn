cmake_minimum_required (VERSION 3.0)

project (master C)

set(DAWN_SOURCES_DIR
    ${CMAKE_CURRENT_LIST_DIR}../../core)

include_directories (${DAWN_SOURCES_DIR})

SET(DAWN_SOURCES
    ${DAWN_SOURCES_DIR}/dawn.c
    )

SET(SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/master.c
    ${DAWN_SOURCES}
    )

add_executable(${PROJECT_NAME} ${SOURCES})
