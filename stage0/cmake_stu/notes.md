一级标题 <Alt+Ctrl+1>二级标题 <Alt+Ctrl+2>三级标题 <Alt+Ctrl+3>四级标题 <Alt+Ctrl+4>五级标题 <Alt+Ctrl+5>六级标题 <Alt+Ctrl+6>

# CMake 工程通用知识点清单

```cmake
# 速查注释（可粘到顶层 CMakeLists 顶部）
# 目标：源外构建、目标为中心、依赖清晰、产物目录可控、可安装/可打包/可测试
# 1) 基本骨架
#   cmake_minimum_required(VERSION 3.20)
#   project(name LANGUAGES CXX)
#   set(CMAKE_CXX_STANDARD 17)        # 或 target_compile_features
#   add_subdirectory(src)             # 按模块拆分
# 2) 目标与使用需求（Usage Requirements）
#   target_include_directories(tgt PUBLIC include)     # 对外 API 头
#   target_link_libraries(app PRIVATE mylib)           # app 依赖 mylib
#   # 作用域：PRIVATE 只当前；PUBLIC 当前+依赖者；INTERFACE 仅依赖者
# 3) 输出与安装
#   set_target_properties(t PROPERTIES
#     RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
#     ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
#     LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
#   install(TARGETS mylib myapp ...)  # 可执行/库/头文件安装
# 4) 构建命令
#   cmake -S . -B build
#   cmake --build build -j --target all
#   cmake --install build --prefix /opt/myproj
```

# 1. 工程目录与源外构建

* 建议结构：`CMakeLists.txt`（顶层）+ `src/`（库与可执行）+ `include/`（对外头）+ `tests/` + `cmake/`（自定义模块）。
* 永远用源外构建：`cmake -S . -B build && cmake --build build -j`。清理只需删 `build/`。

# 2. 顶层最小模板（现代写法）

```cmake
cmake_minimum_required(VERSION 3.20)
project(myproj LANGUAGES CXX)

# 推荐：强制标准、禁用扩展
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(src)
enable_testing() # 如需CTest
```

# 3. 目标与使用需求：PUBLIC / PRIVATE / INTERFACE


| 命令                         | 用途                | 作用域语义                                                      |
| ---------------------------- | ------------------- | --------------------------------------------------------------- |
| `target_include_directories` | 头文件搜索路径      | PUBLIC：当前+依赖者都可见；PRIVATE：仅当前；INTERFACE：仅依赖者 |
| `target_link_libraries`      | 链接依赖（库/目标） | 同上；优先传目标名（mylib），少用裸`-l`                         |
| `target_compile_definitions` | 预定义宏            | 同上                                                            |
| `target_compile_options`     | 编译器选项          | 同上                                                            |
| `target_sources`             | 声明源文件          | PRIVATE/INTERFACE 控制暴露                                      |

核心：**库应通过 PUBLIC/INTERFACE 精准“发布”自己的使用需求**（头路径、编译选项、依赖），上层只需 `target_link_libraries(app mylib)`，无需重复配置 include/flags。

# 4. 常用 target\_\* 命令对照

```cmake
add_library(mylib STATIC src/a.cpp src/b.cpp)
target_include_directories(mylib PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_compile_features(mylib PUBLIC cxx_std_20)   # 或用全局 CMAKE_CXX_STANDARD
target_compile_definitions(mylib PRIVATE MYLIB_INTERNAL=1)
add_executable(myapp src/main.cpp)
target_link_libraries(myapp PRIVATE mylib)
```

# 5. 源文件管理：显式列举 vs GLOB

* 正式项目**显式列出**源文件（利于审查与可控）。
* 练习/小项目可用：
  ```cmake
  file(GLOB SRC CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
  add_library(mylib STATIC ${SRC})
  ```
* 不要用 `aux_source_directory`（老旧且不可递归）。

# 6. 依赖管理

* 首选 **Imported Targets**：`find_package(ZLIB REQUIRED)` → `target_link_libraries(app PRIVATE ZLIB::ZLIB)`。
* 拉取第三方源码：`FetchContent`（内置）或 `ExternalProject`（外部构建）。
* 同仓库内部模块用 `add_subdirectory(anotherlib)`，然后链接库目标名。

# 7. 语言标准与特性

* 项目级：`set(CMAKE_CXX_STANDARD 20)`；更推荐目标级：`target_compile_features(tgt PUBLIC cxx_std_20)`。
* 记得 `CMAKE_CXX_EXTENSIONS OFF`，避免落到 gnu++20。

# 8. 编译/链接选项与符号可见性

* 警告等级等用目标级控制：
  ```cmake
  target_compile_options(tgt PRIVATE -Wall -Wextra -Wpedantic)
  ```
* 导出符号控制（共享库）：
  ```cmake
  set(CMAKE_CXX_VISIBILITY_PRESET hidden)
  set(CMAKE_VISIBILITY_INLINES_HIDDEN YES)
  # 对外 API 用宏标注 default visibility
  ```
* 共享库/PIC：
  ```cmake
  set_target_properties(mylib PROPERTIES POSITION_INDEPENDENT_CODE ON)
  ```

# 9. 产物目录、RPATH 与版本

```cmake
set_target_properties(tgt PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin   # 可执行
  ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib   # 静态库
  LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib   # 共享库
)
# 共享库版本
set_target_properties(mylib PROPERTIES SOVERSION 1 VERSION 1.2.3)
# 运行期找库（ELF）
set_target_properties(myapp PROPERTIES BUILD_RPATH "$ORIGIN/../lib" INSTALL_RPATH "$ORIGIN/../lib")
```

多配置生成器（MSVC/Xcode）用 `..._RELEASE`/`..._DEBUG` 或生成表达式 `$<CONFIG>`。

# 10. 配置/安装/打包/测试

* 配置文件：`configure_file(in.h out.h @ONLY)`（生成含版本号/宏定义的头或配置）。
* 安装：
  ```cmake
  include(GNUInstallDirs)
  install(TARGETS mylib myapp
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
  install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
  ```
* 打包：`include(CPack)` → `cpack -G ZIP`（或 TGZ、DEB、RPM）。
* 测试：`enable_testing()` + `add_test(NAME t1 COMMAND myapp --help)`；`ctest -j`.

# 11. 构建类型与生成器

* 单配置（Make/Ninja）：`-DCMAKE_BUILD_TYPE=Release|Debug|RelWithDebInfo|MinSizeRel`。
* 多配置（MSVC/Xcode）：构建时指定：`cmake --build build --config Release`。
* 导出 `compile_commands.json`：`-DCMAKE_EXPORT_COMPILE_COMMANDS=ON` 便于 IDE/静态分析。

# 12. 变量/Cache/选项/预设

* 命令行传参：`-DVAR=VALUE`（写入 cache）。
* 选项：
  ```cmake
  option(BUILD_SHARED_LIBS "Build shared" OFF)
  if(BUILD_SHARED_LIBS)
    add_library(mylib SHARED ...)
  endif()
  ```
* 预设（推荐）：`CMakePresets.json` 统一参数、生成器、构建目录；`cmake --list-presets` / `cmake --preset <name>`。

# 13. 跨平台要点

* Windows：`.dll`/`.lib`（导入库），无 RPATH，依赖解析常用 PATH 或和可执行同目录；导出符号用 `__declspec(dllexport/dllimport)` 或生成导出文件。
* macOS：`.dylib`，`@rpath/@loader_path`；可用 `install_name_tool` 调整。
* Linux/ELF：`.so`，`RPATH/RUNPATH`、`ldconfig`、`$ORIGIN`。

# 14. 常见错误与排查

* 命令拼写/顺序：`cmake_minimum_required` 必须第一条；`target_*` 必须在 `add_executable/add_library` 之后。
* 把 `${SRC}`（文件列表）当目标名传给 `target_*`（错误）。
* 滥用全局 `include_directories`/`add_definitions` → 用目标级替代。
* 链接顺序/未链接库导致 `undefined reference` → 用目标名链接，避免手工 `-L/-l`。
* 运行时报找不到 `.so` → 配置 `BUILD_RPATH/INSTALL_RPATH` 或设置 `LD_LIBRARY_PATH`。
* GLOB 添加/删除文件未触发重配 → 加 `CONFIGURE_DEPENDS` 或显式列出文件。
* Debug/Release 产物相互覆盖 → 为不同配置设置独立输出目录或用生成表达式。

# 15. 完整示例（库 + 可执行 + 安装）

```cmake
cmake_minimum_required(VERSION 3.20)
project(demo LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(math STATIC src/add.cpp src/mul.cpp)
target_include_directories(math PUBLIC ${CMAKE_SOURCE_DIR}/include)
set_target_properties(math PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

add_executable(app src/main.cpp)
target_link_libraries(app PRIVATE math)
set_target_properties(app PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include(GNUInstallDirs)
install(TARGETS math app
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
```

若你把现有工程发我具体目录与需求（是否要共享库/安装包/测试/第三方依赖），我可以据此给一版“可直接复用”的企业级模板（含 Presets、CPack、CTest、FetchContent 与跨平台处理）。

[ ]

所见即所得 <Alt+Ctrl+7>即时渲染 <Alt+Ctrl+8>分屏预览 <Alt+Ctrl+9>

大纲

DesktopTabletMobile/Wechat
