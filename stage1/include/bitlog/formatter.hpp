// include/bitlog/formatter.hpp
// 概要：最小版格式化器，输出形如
//       [YYYY-mm-dd HH:MM:SS.mmm][LEVEL] message\n
// 用途：把 LogMsg 序列化为一行可读文本。
// 扩展：后续会替换为“可配置 pattern (%d/%p/%m/%n 等)”的版本。
#pragma once
#include <string>
#include "message.hpp"
namespace bitlog
{
    class Formatter
    {
    public:
        Formatter() = default;
        // 把 LogMsg 组装为最终字符串（线程安全：无共享可变状态）
        std::string format(const LogMsg &m) const;
        // const：不修改内部状态；无共享可变状态 → 可被多个线程同时调用。
    };
    // 工具函数：把 time_point 格式化成人类可读的 "YYYY-mm-dd HH:MM:SS.mmm"
    std::string format_time_point(std::chrono::system_clock::time_point tp);
} // namespace bitlog
