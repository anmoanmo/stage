# CMakeLists.txt —— 顶层工程
# 作用：定义 C++17 工程；编译 bitlog 静态库与示例可执行文件
# 关键点：
# 1) 统一开启常见告警（Wall/Wextra/Wpedantic 等）
# 2) 链接 pthread（后续异步会用到）
# 3) 预留 Sanitizer 开关（ASan/UBSan），便于调试时开启
cmake_minimum_required(VERSION 3.20)
project(bitlog LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BITLOG_ENABLE_ASAN  "Enable AddressSanitizer" OFF)
option(BITLOG_ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)
option(BITLOG_BUILD_EXAMPLES "Build examples" ON)

add_library(bitlog STATIC
  src/bitlog.cpp
  src/formatter.cpp
  src/sink.cpp
  src/logger.cpp
)
target_include_directories(bitlog PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_compile_options(bitlog PRIVATE
  -Wall -Wextra -Wpedantic -Wconversion -Wshadow -fno-omit-frame-pointer
)
target_link_libraries(bitlog PUBLIC pthread)

if(BITLOG_ENABLE_ASAN)
  target_compile_options(bitlog PRIVATE -fsanitize=address)
  target_link_options(bitlog PRIVATE -fsanitize=address)
endif()
if(BITLOG_ENABLE_UBSAN)
  target_compile_options(bitlog PRIVATE -fsanitize=undefined)
  target_link_options(bitlog PRIVATE -fsanitize=undefined)
endif()

if(BITLOG_BUILD_EXAMPLES)
  add_executable(example_logger examples/logger.cpp)
  target_link_libraries(example_logger PRIVATE bitlog)
endif()


# ===== Documentation (Doxygen) =====
# 开关：未装 libclang-dev 时保持 OFF
option(BITLOG_DOXYGEN_CLANG "Use clang-assisted parsing in Doxygen" OFF)

set(DOXYFILE_IN  "${CMAKE_SOURCE_DIR}/Doxyfile.in")
set(DOXYFILE_OUT "${CMAKE_BINARY_DIR}/Doxyfile")

# 把布尔量映射为 YES/NO（供 Doxygen 使用）
set(BITLOG_DOXYGEN_CLANG_VAL "NO")
if(BITLOG_DOXYGEN_CLANG)
  set(BITLOG_DOXYGEN_CLANG_VAL "YES")
endif()

configure_file(${DOXYFILE_IN} ${DOXYFILE_OUT} @ONLY)

find_package(Doxygen)
if(DOXYGEN_FOUND)
  set(DOXYGEN_CMD ${DOXYGEN_EXECUTABLE})
else()
  set(DOXYGEN_CMD doxygen)
endif()

add_custom_target(doc
  # 关键：先创建输出目录，避免 Doxygen 报错
  COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/docs/doxygen
  COMMAND ${CMAKE_COMMAND} -E echo "Generating API docs with Doxygen..."
  COMMAND ${DOXYGEN_CMD} ${DOXYFILE_OUT}
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  COMMENT "Doxygen: generating HTML & SVG diagrams to docs/doxygen/html/"
  VERBATIM
)

include(CTest)              # 打开 CTest 支持，定义 BUILD_TESTING 选项（默认 ON）

if(BUILD_TESTING)
  add_executable(formatter_tests
    tests/formatter_tests.cpp
  )
  target_link_libraries(formatter_tests PRIVATE bitlog)
  add_test(NAME formatter_tests COMMAND formatter_tests)
endif()
