cmake_minimum_required(VERSION 3.20)
project(SORTLIB LANGUAGES CXX)

file(GLOB SRC CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)

add_library(${SORT_LIB} STATIC ${SRC})
target_include_directories(${SORT_LIB} PUBLIC ${HEAD_PATH})

set_target_properties(${SORT_LIB} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${LIB_PATH}
)
