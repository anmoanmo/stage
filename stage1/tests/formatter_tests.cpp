// tests/formatter_tests.cpp
// 目的：只依赖标准库 + 断言，验证 formatter 的核心行为。
// 覆盖：时间格式形状、毫秒零填充/截断、行格式、嵌入 '\0'、并发安全（冒烟）。

#include <cassert>
#include <chrono>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include <atomic>
#include "bitlog/formatter.hpp"
#include "bitlog/level.hpp"
#include "bitlog/message.hpp"

// ---- 平台辅助：localtime 安全变体 ----
static inline void localtime_safe(const std::time_t *t, std::tm *out)
{
#ifdef _WIN32
    localtime_s(out, t);
#else
    localtime_r(t, out);
#endif
}

// ---- 测试工具：构造一个“本地时区下 y-m-d h:m:s.ms 对应的 time_point” ----
// 做法：先把一个“本地 tm”喂给 mktime 得到 time_t（本地→time_t），
// 再 + 毫秒转为 system_clock::time_point。
static std::chrono::system_clock::time_point
make_tp_local(int y, int mon, int d, int h, int mi, int s, int ms)
{
    std::tm tm{};
    tm.tm_year = y - 1900;
    tm.tm_mon = mon - 1;
    tm.tm_mday = d;
    tm.tm_hour = h;
    tm.tm_min = mi;
    tm.tm_sec = s;
    tm.tm_isdst = -1;                  // 让库自行判断 DST
    std::time_t tt = std::mktime(&tm); // 解释为“本地时间”
    return std::chrono::system_clock::from_time_t(tt) + std::chrono::milliseconds(ms);
}

// ---- 测试工具：用与被测代码相同的规则构造“期望的时间字符串” ----
// 先把 time_t -> local tm，再用 put_time + 手工毫秒拼接。
static std::string expect_time_string(std::chrono::system_clock::time_point tp)
{
    using namespace std::chrono;
    std::time_t tt = system_clock::to_time_t(tp);
    auto us = duration_cast<microseconds>(tp.time_since_epoch()) % seconds(1);

    std::tm tm{};
    localtime_safe(&tt, &tm);

    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S")
        << '.' << std::setw(3) << std::setfill('0') << (us.count() / 1000);
    return oss.str();
}

// ---- 断言工具：字符串以 prefix 开头 ----
static void assert_starts_with(const std::string &s, const std::string &prefix)
{
    assert(s.size() >= prefix.size());
    assert(s.compare(0, prefix.size(), prefix) == 0);
}

int main()
{
    using namespace std::chrono;

    // ========== 用例 1：format_time_point 的形状与毫秒零填充 ==========
    {
        auto tp = make_tp_local(2000, 1, 2, 3, 4, 5, 7); // 2000-01-02 03:04:05.007 本地时间
        std::string got = bitlog::format_time_point(tp);
        std::string exp = expect_time_string(tp);
        // 完全相等（同一算法计算的期望，无时区依赖）
        assert(got == exp && "time string should match expected with zero padding");
    }
    {
        auto tp = make_tp_local(2000, 1, 2, 3, 4, 5, 45);
        assert(bitlog::format_time_point(tp).substr(0, 23) // 到秒
               == expect_time_string(tp).substr(0, 23));
        assert(bitlog::format_time_point(tp).substr(20) // ".045"
                   .find(".045") != std::string::npos);
    }
    {
        auto tp = make_tp_local(2000, 1, 2, 3, 4, 5, 123);
        auto s = bitlog::format_time_point(tp);
        assert(s.find(".123") != std::string::npos);
    }

    // ========== 用例 2：秒边界（05.999 -> 06.000） ==========
    {
        auto tp1 = make_tp_local(2000, 1, 2, 3, 4, 5, 999);
        auto tp2 = tp1 + milliseconds(1);
        auto s1 = bitlog::format_time_point(tp1);
        auto s2 = bitlog::format_time_point(tp2);
        // 到“秒”的文本应不同
        assert(s1.substr(0, 19) != s2.substr(0, 19));
        // 毫秒应为 ".999" 与 ".000"
        assert(s1.find(".999") != std::string::npos);
        assert(s2.find(".000") != std::string::npos);
    }

    // ========== 用例 3：Formatter::format 的整体形状 ==========
    {
        bitlog::Formatter f;
        bitlog::LogMsg m;
        m.name = "root";
        m.level = bitlog::Level::INFO;
        m.time = make_tp_local(2000, 1, 2, 3, 4, 5, 678);
        m.payload = "hello";

        std::string line = f.format(m);
        // 1) 以 "[YYYY-.." 开头
        assert(line.size() >= 2 && line[0] == '[');
        // 2) 前缀应等于 expect_time_string + "][INFO] "
        std::string time_prefix = expect_time_string(m.time);
        std::string prefix = "[" + time_prefix + "][INFO] ";
        assert_starts_with(line, prefix);
        // 3) 正文与换行
        assert(line.find("hello\n") == line.size() - 6);
    }

    // ========== 用例 4：payload 内含 '\0'（二进制安全） ==========
    {
        bitlog::Formatter f;
        bitlog::LogMsg m;
        m.name = "root";
        m.level = bitlog::Level::ERROR;
        m.time = std::chrono::system_clock::now();
        m.payload = std::string("A\0B", 3); // 含 NUL 字节的字符串

        std::string line = f.format(m);
        // 确保 line 内部确有 '\0'，并且末尾是 '\n'
        assert(line.back() == '\n');
        // 找到 Level 之后的空格位置
        auto pos = line.find("] ") + 2;
        // 检查 payload 的三字节 A \0 B 被完整保留
        assert(pos != std::string::npos);
        assert(line[pos] == 'A');
        assert(line[pos + 1] == '\0');
        assert(line[pos + 2] == 'B');
    }

    // ========== 用例 5：简单并发冒烟（线程安全） ==========
    {
        bitlog::Formatter f;
        const int threads = 8;
        const int iters = 5000;
        std::atomic<int> ok{0};

        auto worker = [&]
        {
            for (int i = 0; i < iters; ++i)
            {
                bitlog::LogMsg m;
                m.name = "T";
                m.level = bitlog::Level::DEBUG;
                m.time = std::chrono::system_clock::now();
                m.payload = "x";
                auto line = f.format(m);
                // 基本形状检查
                if (!line.empty() && line.back() == '\n' && line.find("][DEBUG] ") != std::string::npos)
                {
                    ok.fetch_add(1, std::memory_order_relaxed);
                }
            }
        };

        std::vector<std::thread> v;
        v.reserve(threads);
        for (int i = 0; i < threads; ++i)
            v.emplace_back(worker);
        for (auto &t : v)
            t.join();

        assert(ok == threads * iters);
    }

    std::cout << "[formatter_tests] all tests passed\n";
    return 0;
}
