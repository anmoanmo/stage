目标：提供“同步/异步、可扩展 Sink、可配置格式”的轻量日志库，并配套示例与基准脚本
核心模块：

- level.hpp：日志等级枚举与字符串化
- message.hpp：结构化日志载荷（时间、线程、文件:行、级别、消息、logger 名）
- formatter.hpp：pattern 解析与格式化（%d/%t/%p/%c/%f/%l/%m/%n/%T）
- sink.hpp：StdoutSink/FileSink/RollSink（按大小滚动）
- buffer.hpp：线性 Buffer（单次写单次读的双缓冲使用）
- looper.hpp：AsyncLooper（前端 push、后台线程批量回调）
- logger.hpp：Logger 抽象 + SyncLogger/AsyncLogger；Builder + 全局注册中心
- bitlog.h：入口与日志宏（LOG_DEBUG/INFO/WARN/ERROR/FATAL、LOGD/LOGI/…）
  示例/基准：
- example/logger.cc：多 Sink + 异步示例，含 100 万条写入压力
- bench：并发写入基准（比较同步/异步）

# `formatter`

```cpp
#include <ctime>      // time_t、struct tm、本地时间转换(localtime_* / gmtime_*)
#include <iomanip>    // std::put_time、std::setw、std::setfill 等流格式控制
#include <sstream>    // std::ostringstream 字符串输出流（在内存里拼接字符串）
#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS  // MSVC: 允许使用部分 C 函数而不触发“非安全”告警
#endif
#include "bitlog/formatter.hpp"   // 本模块的声明（Formatter、format_time_point）
#include "bitlog/level.hpp"       // to_string(Level) —— 等级转文本

namespace bitlog {

// 把“日历时间点（system_clock::time_point）”格式化成 "YYYY-mm-dd HH:MM:SS.mmm"
std::string format_time_point(std::chrono::system_clock::time_point tp) {
  using namespace std::chrono;

  // 1) 转为 time_t（单位：秒）。这是 C 时代常用的“日历时间”表示
  const auto t = system_clock::to_time_t(tp);

  // 2) 取“这一秒内的亚秒部分”。先把 epoch 起点到 now 的微秒数取模 1 秒，
  //    得到 [0, 1s) 的 microseconds，再 /1000 得到毫秒（截断，不四舍五入）
  const auto us = duration_cast<microseconds>(tp.time_since_epoch()) % seconds(1);

  // 3) 把 time_t（秒）分解成本地时间各字段 (year, mon, day, hour,...)
  //    注意: 不能用 localtime(非 *_s/_r)，它内部用静态缓冲，不是线程安全的
  std::tm tm{};  // POD 结构体，注意要初始化
#ifdef _WIN32
  localtime_s(&tm, &t);  // Windows 安全版，签名: errno_t localtime_s(tm* out, const time_t* in)
#else
  localtime_r(&t, &tm);  // POSIX 安全版，签名: tm* localtime_r(const time_t* in, tm* out)
#endif

  // 4) 用 iostream 把 "YYYY-mm-dd HH:MM:SS" 写到内存流，再手工拼 ".mmm"
  //    - std::put_time(&tm, fmt) : 以 strftime 的格式把 tm 写入流
  //    - std::setw(3)            : 指定下一个输出项宽度为 3
  //    - std::setfill('0')       : 不足位用 '0' 填充（setfill 会保留在流状态里）
  std::ostringstream oss;
  oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S")
      << '.' << std::setw(3) << std::setfill('0')
      << (us.count() / 1000); // 毫秒，000..999（截断）
  return oss.str();           // 生成最终字符串（可能发生一次内存分配）
}

// 把一条结构化日志 LogMsg 按固定样式拼成一行：
//   [YYYY-mm-dd HH:MM:SS.mmm][LEVEL] payload\n
std::string Formatter::format(const LogMsg &m) const {
  // ostringstream 是“字符串输出流”：像向 cout 打印一样，向内存缓冲写
  std::ostringstream oss;

  // 前缀: 方括号包裹的时间和等级
  // - format_time_point(m.time) : 本地时区的时间字符串
  // - to_string(m.level)        : 等级枚举转常量字符串(如 "INFO")
  oss << '[' << format_time_point(m.time) << ']'
      << '[' << to_string(m.level) << "] ";

  // 正文与行末换行
  oss << m.payload << '\n';

  // 返回最终一行文本
  return oss.str();
}

} // namespace bitlog
```

---

## 工作原理（一步步串起来）

1. `format_time_point(tp)` 做四件事：
   * `system_clock::to_time_t(tp)`：把时间点降为**秒**（`time_t`），是 C API 的通用输入。
   * 计算毫秒：用 `time_since_epoch() % 1s` 拿到“当前秒内的微秒”，再 `/ 1000` 得**毫秒**（**截断**）。
   * `localtime_s/_r`：把 `time_t` 分解为**本地时区**的 `tm`（**线程安全**变体）。
   * `ostringstream + put_time + setw(3)+setfill('0')`：输出“年月日时分秒”和 3 位零填充的毫秒。
2. `Formatter::format(m)`：
   * 先拼 `[时间][等级]` 前缀（时间来自上面的函数，等级来自 `to_string(Level)`）。
   * 接正文 `payload`，最后补一个 `'\n'`。
   * 返回最终整行字符串。

这俩函数都**不共享可变状态**，因此**线程安全**，可以被多个线程同时调用。

---

## 这里用到的“非常用 API”速查

* `std::put_time(const std::tm*, const char*)`（`<iomanip>`）：把 `tm` 按 strftime 风格写入流。
* `std::setw(int)` + `std::setfill(char)`（`<iomanip>`）：控制**下一个**输出项的最小宽度与填充字符。
* `localtime_s` / `localtime_r`（`<ctime>`/平台相关）：把 `time_t` 转成本地 `tm`（**线程安全**）。
  * Windows：`localtime_s(out_tm, &t)`
  * POSIX：`localtime_r(&t, out_tm)`
* `std::ostringstream`（`<sstream>`）：内存里的输出流；像对 `cout` 一样 `<<`，最后 `str()`/隐式转化拿字符串。
* `std::chrono` 基本用法：
  * `system_clock::now()` 得到**日历时间**；
  * `duration_cast<microseconds>(tp.time_since_epoch()) % seconds(1)` 拿“当秒内微秒数”。

---

## 易错点 & 解释

1. **“为什么不用 `localtime`（没有 `_s/_r`）？”**
   因为它返回的 `tm*` 指向**内部静态缓冲**，多线程并发会数据竞态。`localtime_s/_r` 用**调用者提供的缓冲**，是线程安全的。
2. **毫秒是“截断”不是“四舍五入”**
   `(us.count() / 1000)` 会直接砍掉 0.5 毫秒以上的部分；若想四舍五入，改成：
   ```cpp
   auto ms = duration_cast<milliseconds>(tp.time_since_epoch() + microseconds(500)) % seconds(1);
   oss << std::setw(3) << std::setfill('0') << ms.count();
   ```
3. **`std::setfill` 会留在流状态里**
   我们这里只对一个“`<<` 紧随其后的整数”生效，之后没再用到 `setw`，所以不会有副作用。但如果后面还要格式化别的字段，建议再 `std::setfill(' ')` 复原。
4. **这是“本地时间”不是 UTC**
   切换到 UTC 只需要把 `localtime_*` 改 `gmtime_*`（同样有 `_s/_r` 变体）即可：
   ```cpp
   #ifdef _WIN32
     gmtime_s(&tm, &t);
   #else
     gmtime_r(&t, &tm);
   #endif
   ```
5. **`system_clock` 不是单调的**
   它跟随系统时间、时区/DST 调整，适合“展示时间”；**不要用它做耗时统计**（用 `steady_clock`）。

---

## 可改进（按需选择）

* **更快**：用 C 的 `strftime + snprintf`，一次在栈上写入固定大小缓冲，零额外分配。
* **更现代**：用 `{fmt}` 库（或 C++20 `<format>`）：
  ```cpp
  auto s = fmt::format("{:%Y-%m-%d %H:%M:%S}.{:03}",
                       fmt::localtime(std::chrono::system_clock::to_time_t(tp)),
                       (std::chrono::duration_cast<std::chrono::microseconds>(
                         tp.time_since_epoch()) % std::chrono::seconds(1)).count() / 1000);
  ```
* **不分配版本**：给 `Formatter` 增加重载，把结果写到调用者提供的 `std::string& out` 或 `std::ostream&`.

---

## 小测试建议

1. **形状测试**：断言输出匹配 `^\[\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\.\d{3}\]\[(DEBUG|INFO|WARN|ERROR|FATAL|OFF)\] .*\n$`。
2. **零填充**：构造不同毫秒（7、45、123），检查 `.007/.045/.123`。
3. **并发**：多线程循环 `format()`，配合 TSAN 运行，应无数据竞争与崩溃。
4. **UTC对比**：把 `localtime_*` 改 `gmtime_*`，同一 `time_point` 输出应不同（时区差）。


# `Sink`
