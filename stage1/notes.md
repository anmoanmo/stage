# 第 1 步｜最小可运行版（同步 Logger + CMake 脚手架）

## 1. CMake 脚手架

```cmake
# CMakeLists.txt —— 顶层工程
# 作用：定义 C++17 工程；编译 bitlog 静态库与示例可执行文件
# 关键点：
# 1) 统一开启常见告警（Wall/Wextra/Wpedantic 等）
# 2) 链接 pthread（后续异步会用到）
# 3) 预留 Sanitizer 开关（ASan/UBSan），便于调试时开启
cmake_minimum_required(VERSION 3.16)
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
```

目录结构建议：

```
stage1/
├─ CMakeLists.txt
├─ include/bitlog/*.hpp
├─ src/*.cpp
└─ examples/logger.cpp
```

---

## 2. 等级与消息模型

```cpp
// include/bitlog/level.hpp
// 概要：日志等级枚举与字符串化。
// 用途：控制是否输出；格式化时打印等级文本。
// 语义：等级越高表示越严重；OFF 表示关闭输出。
// 注意：to_string 对未知值返回 "UNKNOW"，不要依赖枚举的具体整数值做跨模块协议。
#pragma once
namespace bitlog {
enum class Level : int { DEBUG=0, INFO, WARN, ERROR, FATAL, OFF };

inline const char* to_string(Level lv) {
  switch (lv) {
    case Level::DEBUG: return "DEBUG";
    case Level::INFO:  return "INFO";
    case Level::WARN:  return "WARN";
    case Level::ERROR: return "ERROR";
    case Level::FATAL: return "FATAL";
    case Level::OFF:   return "OFF";
    default:           return "UNKNOW"; // 实现相关：防御性返回
  }
}
} // namespace bitlog
```

```cpp
// include/bitlog/message.hpp
// 概要：一条日志的结构化载荷（最小集合）。
// 字段：name(记录器名)、level(级别)、time(发生时间点)、payload(格式化后的正文)。
// 用途：Formatter 将 LogMsg 转换为最终字符串，Sink 负责将字符串落地。
// 注意：time 使用 std::chrono::system_clock::time_point；跨时区展示与格式化由 Formatter 决定。
#pragma once
#include <string>
#include <chrono>
#include "level.hpp"
namespace bitlog {
struct LogMsg {
  std::string name;                                     // 日志器名（可选）
  Level level{};                                        // 级别
  std::chrono::system_clock::time_point time{};         // 事件时间
  std::string payload;                                  // 正文（已由 Logger 变参格式化而来）
};
} // namespace bitlog
```

---

## 3. Formatter（固定格式最小实现）

```cpp
// include/bitlog/formatter.hpp
// 概要：最小版格式化器，输出形如
//       [YYYY-mm-dd HH:MM:SS.mmm][LEVEL] message\n
// 用途：把 LogMsg 序列化为一行可读文本。
// 扩展：后续会替换为“可配置 pattern (%d/%p/%m/%n 等)”的版本。
#pragma once
#include <string>
#include "message.hpp"
namespace bitlog {
class Formatter {
public:
  Formatter() = default;
  // 把 LogMsg 组装为最终字符串（线程安全：无共享可变状态）
  std::string format(const LogMsg& m) const;
};
// 工具函数：把 time_point 格式化成人类可读的 "YYYY-mm-dd HH:MM:SS.mmm"
std::string format_time_point(std::chrono::system_clock::time_point tp);
} // namespace bitlog
```

```cpp
// src/formatter.cpp
// 说明：使用 localtime_r/localtime_s 做线程安全的本地时间转换；
//       使用 iostream + put_time + 手动毫秒拼接。
#include <iomanip>
#include <sstream>
#include <ctime>
#ifdef _WIN32
  #define _CRT_SECURE_NO_WARNINGS
#endif
#include "bitlog/formatter.hpp"
#include "bitlog/level.hpp"

namespace bitlog {
std::string format_time_point(std::chrono::system_clock::time_point tp) {
  using namespace std::chrono;
  const auto t  = system_clock::to_time_t(tp);                  // 秒
  const auto us = duration_cast<microseconds>(tp.time_since_epoch()) % seconds(1);
  std::tm tm{};
#ifdef _WIN32
  localtime_s(&tm, &t);     // Windows 线程安全接口（标准库扩展）
#else
  localtime_r(&t, &tm);     // POSIX 线程安全接口
#endif
  std::ostringstream oss;
  oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S")
      << '.' << std::setw(3) << std::setfill('0') << (us.count() / 1000); // 毫秒
  return oss.str();
}

std::string Formatter::format(const LogMsg& m) const {
  std::ostringstream oss;
  // 统一前缀：[时间][等级]
  oss << '[' << format_time_point(m.time) << ']'
      << '[' << to_string(m.level) << "] ";
  // 正文与行末换行
  oss << m.payload << '\n';
  return oss.str();
}
} // namespace bitlog
```

### 讲解（Formatter）

* 标准与实现：`localtime_r`/`localtime_s` 都是实现相关接口（POSIX/Windows），都能避免 `localtime` 的静态共享缓冲带来的数据竞争。跨平台要做条件编译，本实现已处理。
* 时区：此处输出为**本地时区**。如果你的系统设为 UTC，显示即为 UTC。后续可扩展 `%z/%Z`。
* 小数位：通过 `time_since_epoch()%1s` 取得亚秒，再手动取毫秒，避免 iostream 的复杂性。

---

## 4. Sink（输出后端）

```cpp
// include/bitlog/sink.hpp
// 概要：抽象落地器接口 + 标准输出实现。
// 用途：后端可以替换为文件、滚动文件、网络等；本步仅提供 StdoutSink。
#pragma once
#include <memory>
#include <string>
namespace bitlog {
struct LogSink {
  using ptr = std::shared_ptr<LogSink>;
  virtual ~LogSink() = default;
  // 约定：接收“完整的一行字符串”（含或不含换行均可，具体由 Formatter 决定）
  virtual void log(const std::string& text) = 0;
};
struct StdoutSink final : LogSink {
  void log(const std::string& text) override;
};
} // namespace bitlog
```

```cpp
// src/sink.cpp
// 说明：使用 cout.write 避免额外格式化；立即 flush 方便调试观察。
#include <iostream>
#include "bitlog/sink.hpp"
namespace bitlog {
void StdoutSink::log(const std::string& text) {
  std::cout.write(text.data(), static_cast<std::streamsize>(text.size()));
  std::cout.flush();
}
} // namespace bitlog
```

---

## 5. Logger（同步版本）

```cpp
// include/bitlog/logger.hpp
// 概要：同步 Logger，提供 debug/info/warn/error/fatal 变参接口；
//       内部用互斥串行写入所有 Sink；
//       使用 vsnprintf 两段式进行安全格式化（不依赖 GNU vasprintf）。
// 线程安全：同一 Logger 实例可被多个线程并发调用；内部互斥保证 Sink 串行输出。
// 注意：本步未携带 __FILE__/__LINE__/线程ID 等字段，后续再增强。
#pragma once
#include <atomic>
#include <memory>
#include <mutex>
#include <vector>
#include <cstdarg>
#include "level.hpp"
#include "formatter.hpp"
#include "sink.hpp"

namespace bitlog {
class Logger {
public:
  using ptr = std::shared_ptr<Logger>;
  explicit Logger(std::string name);
  void set_level(Level lv) { _level.store(lv, std::memory_order_relaxed); }
  Level level() const { return _level.load(std::memory_order_relaxed); }
  void add_sink(LogSink::ptr s);
  void set_formatter(std::shared_ptr<Formatter> f);

  // 变参接口（C 风格，可与 printf 一致；后续替换为 {fmt} 可选）
  void debug(const char* fmt, ...);
  void info (const char* fmt, ...);
  void warn (const char* fmt, ...);
  void error(const char* fmt, ...);
  void fatal(const char* fmt, ...);

  const std::string& name() const { return _name; }

private:
  void vlog(Level lv, const char* fmt, va_list ap); // 统一入口
  void log_it(const std::string& text);             // 串行写入 sinks
  static bool enabled(Level cur, Level req) {
    // 语义：当 req 等级“高于等于”当前过滤等级且当前不是 OFF 时输出
    return static_cast<int>(req) >= static_cast<int>(cur) && cur != Level::OFF;
  }

private:
  std::string _name;                       // 记录器名
  std::atomic<Level> _level{Level::DEBUG}; // 原子可并发读写
  std::vector<LogSink::ptr> _sinks;        // 输出后端集合
  std::shared_ptr<Formatter> _formatter;   // 格式化器
  std::mutex _mutex;                       // 串行化保护
};

// 工具函数：vsnprintf 两段式，安全构造 std::string
std::string vformat(const char* fmt, va_list ap);
} // namespace bitlog
```

```cpp
// src/logger.cpp
#include <vector>
#include <cassert>
#include <cstdio>
#include "bitlog/logger.hpp"

namespace bitlog {

Logger::Logger(std::string name)
  : _name(std::move(name)),
    _formatter(std::make_shared<Formatter>()) {
  _sinks.reserve(1); // 小优化：默认至少一个 sink
}

void Logger::add_sink(LogSink::ptr s) { _sinks.push_back(std::move(s)); }
void Logger::set_formatter(std::shared_ptr<Formatter> f) { _formatter = std::move(f); }

// 五个等级的变参接口：统一走 vlog
void Logger::debug(const char* fmt, ...) { va_list ap; va_start(ap, fmt); vlog(Level::DEBUG, fmt, ap); va_end(ap); }
void Logger::info (const char* fmt, ...) { va_list ap; va_start(ap, fmt); vlog(Level::INFO , fmt, ap); va_end(ap); }
void Logger::warn (const char* fmt, ...) { va_list ap; va_start(ap, fmt); vlog(Level::WARN , fmt, ap); va_end(ap); }
void Logger::error(const char* fmt, ...) { va_list ap; va_start(ap, fmt); vlog(Level::ERROR, fmt, ap); va_end(ap); }
void Logger::fatal(const char* fmt, ...) { va_list ap; va_start(ap, fmt); vlog(Level::FATAL, fmt, ap); va_end(ap); }

void Logger::vlog(Level lv, const char* fmt, va_list ap) {
  if (!enabled(_level.load(std::memory_order_relaxed), lv)) return;

  // 1) 先用 vsnprintf 生成 payload（C 风格，printf 兼容）
  std::string payload = vformat(fmt, ap);

  // 2) 组装结构化消息
  LogMsg msg;
  msg.name    = _name;
  msg.level   = lv;
  msg.time    = std::chrono::system_clock::now();
  msg.payload = std::move(payload);

  // 3) Formatter 输出最终文本
  std::string text = _formatter->format(msg);

  // 4) 串行写入所有 Sink
  log_it(text);
}

void Logger::log_it(const std::string& text) {
  std::lock_guard<std::mutex> lk(_mutex); // 实现相关：同一 Logger 内部串行
  for (auto& s : _sinks) s->log(text);
}

// vsnprintf 两段式：先求长度，再一次性填充
// 注意：这是实现相关行为；当格式串与实参类型不匹配时，行为未定义（与 printf 一致）。
std::string vformat(const char* fmt, va_list ap) {
  va_list ap2; va_copy(ap2, ap);
  const int n = std::vsnprintf(nullptr, 0, fmt, ap2); // 计算需要的字节数（不含 '\0'）
  va_end(ap2);
  if (n <= 0) return std::string();                   // 防御：异常或空
  std::string buf(static_cast<size_t>(n), '\0');      // 精确分配
  std::vsnprintf(buf.data(), static_cast<size_t>(n) + 1, fmt, ap);
  return buf;                                         // 注意：buf 不含终止符
}
} // namespace bitlog
```

### 讲解（Logger）

* 等级过滤：比较的是整数顺序。不要依赖 Level 枚举的“数值大小”在不同编译单元中保持一致做外部协议。
* 线程安全：同一 Logger 内部通过互斥串行化；多个 Logger 各自独立锁，不相互影响。
* 变参安全：`printf` 风格对“格式串与实参类型不匹配”属于未定义行为；后续可以把实现切到 `{fmt}` 或 `<format>` 获得编译期更强校验。

---

## 6. root 记录器与宏

```cpp
// include/bitlog/bitlog.h
// 概要：提供进程级 root() 与若干便捷宏。
// 用途：不显式创建 Logger 时可直接 LOGI/LOGE 输出；root 缺省接 StdoutSink。
#pragma once
#include "logger.hpp"
namespace bitlog {
Logger& root(); // 进程级单例（Meyers Singleton）
} // namespace bitlog

// 便捷宏：走 root() 的快捷等级接口
#define LOGD(fmt, ...) ::bitlog::root().debug(fmt, ##__VA_ARGS__)
#define LOGI(fmt, ...) ::bitlog::root().info (fmt, ##__VA_ARGS__)
#define LOGW(fmt, ...) ::bitlog::root().warn (fmt, ##__VA_ARGS__)
#define LOGE(fmt, ...) ::bitlog::root().error(fmt, ##__VA_ARGS__)
#define LOGF(fmt, ...) ::bitlog::root().fatal(fmt, ##__VA_ARGS__)
```

```cpp
// src/bitlog.cpp
// 说明：Meyers Singleton。初始化阶段给 root 添加 StdoutSink 并设置默认等级。
#include "bitlog/bitlog.h"
namespace bitlog {
Logger& root() {
  static Logger logger("root");       // C++11 起线程安全
  static bool inited = []{
    logger.add_sink(std::make_shared<StdoutSink>());
    logger.set_level(Level::DEBUG);
    return true;
  }();
  (void)inited;                       // 防止未使用告警
  return logger;
}
} // namespace bitlog
```

---

## 7. 示例程序

```cpp
// examples/logger.cpp
// 目标：验证最小链路是否能跑通；观察控制台输出一行 INFO 日志。
#include "bitlog/bitlog.h"
int main() {
  auto& lg = bitlog::root();                  // 取 root 记录器（stdout）
  lg.info("hello %s %d", "bitlog", 42);       // 成员方法调用
  LOGI("root macro %s=%d", "answer", 42);     // 宏调用
  return 0;
}
```

---

## 8. 构建与运行

```bash
cd stage1
cmake -S . -B build -DBITLOG_BUILD_EXAMPLES=ON -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build -j
./build/example_logger
# 期望输出：
# [2025-08-10 09:00:00.123][INFO] hello bitlog 42
# [2025-08-10 09:00:00.124][INFO] root macro answer=42
```

---

## 9. 自检清单（本步）

* 能否在不同线程下并发调用 `lg.info()` 而不出现交错写入一半的情况（应当不会，因每行字符串作为整体写入，并有互斥保证）。
* 把 `logger.set_level(bitlog::Level::WARN);` 后再写 `info()`，是否被正确过滤。
* 在 Windows 与 Linux/WSL 上均可编译运行（`localtime_s/localtime_r` 分支已处理）。

---

## 10. 下一步预告（你确认本步通过后我继续推）

1. 增加 `FileSink` 与 `RollSink`（按大小滚动，文件名零填充，按时间排序稳定）。
2. `Formatter` 改为**可配置 pattern**，先支持 `%d/%p/%m/%n` 的子集，并保留默认格式。
3. 引入 `LoggerManager + Builder`，支持命名 Logger 与全局注册获取。
4. 实现 `AsyncLooper + AsyncLogger`，讲清楚**背压、容量上限、停机唤醒**等关键并发点。
5. 添加 `bench` 与 Sanitizer/TSAN 验证清单。

如果你已经把以上代码跑通，把编译输出/运行截图或遇到的告警贴出来；我会据此确认环境，然后推进“FileSink + RollSink”的下一步，并继续保持这种“代码已注释 + 语义讲解”的节奏。
