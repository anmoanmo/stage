CMAKE_MINIMUM_REQUIRED(VERSION 3.20)
project(camke_stu)

set(LIB_PATH ${CMAKE_SOURCE_DIR}/lib)
set(EXEC_PATH ${CMAKE_SOURCE_DIR}/bin)
set(HEAD_PATH ${CMAKE_SOURCE_DIR}/include)

set(CALC_LIB calc)
set(SORT_LIB sort)

set(APP_NAME_1 test1)
set(APP_NAME_2 test2)

add_subdirectory(calc)
add_subdirectory(sort)
add_subdirectory(test1)
add_subdirectory(test2)

