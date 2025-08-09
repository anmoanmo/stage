cmake_minimum_required(VERSION 3.20)
project(CALC_TEST LANGUAGES CXX)

# 1) 收集本目录源文件
file(GLOB SRC CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)

# 2) 先创建可执行目标
add_executable(${APP_NAME_1} ${SRC})

# 3) 连接 calc 库（同工程内 CMake 目标，推荐做法）
target_link_libraries(${APP_NAME_1} PRIVATE ${CALC_LIB})

# 如 calc 目标已经：
# target_include_directories(calc PUBLIC ${HEAD_PATH})
# 则 test1 会自动继承头路径，这里无需再写 include。如果没有，则启用下一行：
# target_include_directories(${APP_NAME_1} PRIVATE ${HEAD_PATH})

# 4) 可执行产物输出目录
set_target_properties(${APP_NAME_1} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${EXEC_PATH}
)

# 一般不需要：
# target_link_directories(${APP_NAME_1} PUBLIC ${LIB_PATH})
# 只有在你手工链接某个非 CMake 目标的 .a/.so 且放在自定义目录时，才考虑使用。
