cmake_minimum_required(VERSION 3.20)
project(CALCLIB LANGUAGES CXX)

file(GLOB SRC CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)

add_library(${CALC_LIB} STATIC ${SRC})
target_include_directories(${CALC_LIB} PUBLIC ${HEAD_PATH})

set_target_properties(${CALC_LIB} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${LIB_PATH}
)
