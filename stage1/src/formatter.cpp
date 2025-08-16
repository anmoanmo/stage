// 目标：把 std::chrono::system_clock::time_point → 人类可读的本地时间字符串；
//       然后按 “[时间][等级] 消息\n” 的固定样式拼接整行日志。
// 关键点：
// 1) 线程安全的本地时区分解：Windows 用 localtime_s，POSIX 用
// localtime_r（避免非线程安全的 localtime）。
// 2) 毫秒打印：从
// time_since_epoch() 取微秒 % 1s，整数除以 1000 得毫秒；setw(3)+setfill('0')
// 保证零填充。
// 3) iostream 格式化：std::put_time 负责 “YYYY-mm-dd
// HH:MM:SS”；std::ostringstream 组装最终字符串。
// 4) 语义注意：system_clock
// 是“日历时间”，受系统时区/DST 影响；用于展示 OK，但不适合做耗时统计（应使用
// steady_clock）。
// 5) 可移植/可扩展：若要 UTC 输出、更多占位或更高性能，可切换
// strftime/snprintf 或 {fmt}/C++20 <format>。

#include <ctime>   // time_t、struct tm、本地时间转换(localtime_* / gmtime_*)
#include <iomanip> // std::put_time、std::setw、std::setfill 等流格式控制
#include <sstream> // std::ostringstream 字符串输出流（在内存里拼接字符串）
#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS // MSVC: 允许使用部分 C 函数而不触发“非安全”告警
#endif
#include "bitlog/formatter.hpp" // 本模块的声明（Formatter、format_time_point）
#include "bitlog/level.hpp"     // to_string(Level) —— 等级转文本

namespace bitlog
{

    // 把“日历时间点（system_clock::time_point）”格式化成 "YYYY-mm-dd HH:MM:SS.mmm"
    std::string format_time_point(std::chrono::system_clock::time_point tp)
    {
        using namespace std::chrono;

        // 1) 转为 time_t（单位：秒）。这是 C 时代常用的“日历时间”表示
        const auto t = system_clock::to_time_t(tp);

        // 2) 取“这一秒内的亚秒部分”。先把 epoch 起点到 now 的微秒数取模 1 秒，
        //    得到 [0, 1s) 的 microseconds，再 /1000 得到毫秒（截断，不四舍五入）
        const auto us =
            duration_cast<microseconds>(tp.time_since_epoch()) % seconds(1);

        // 3) 把 time_t（秒）分解成本地时间各字段 (year, mon, day, hour,...)
        //    注意: 不能用 localtime(非 *_s/_r)，它内部用静态缓冲，不是线程安全的
        std::tm tm{}; // POD 结构体，注意要初始化
#ifdef _WIN32
        localtime_s(&tm, &t); // Windows 安全版，签名: errno_t localtime_s(tm* out,
                              // const time_t* in)
#else
        localtime_r(
            &t,
            &tm); // POSIX 安全版，签名: tm* localtime_r(const time_t* in, tm* out)
#endif

        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S")
            << '.' << std::setw(3) << std::setfill('0')
            << (us.count() / 1000); // 毫秒，000..999（截断）
        return oss.str();           // 减少内存分配次数
    }

    std::string Formatter::format(const LogMsg &m) const
    {
        std::ostringstream oss;

        oss << '[' << format_time_point(m.time) << ']'
            << '[' << to_string(m.level) << ']';

        oss << m.payload << '\n';

        return oss.str();
    }

} // namespace bitlog
