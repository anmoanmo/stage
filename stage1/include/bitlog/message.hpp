// include/bitlog/message.hpp
// 概要：一条日志的结构化载荷（最小集合）。
// 字段：name(记录器名)、level(级别)、time(发生时间点)、payload(格式化后的正文)。
// 用途：Formatter 将 LogMsg 转换为最终字符串，Sink 负责将字符串落地。
// 注意：time 使用 std::chrono::system_clock::time_point；跨时区展示与格式化由 Formatter 决定。
#pragma once
#include <string>
#include <chrono>
#include "level.hpp"
namespace bitlog
{
    struct LogMsg
    {
        std::string name;                             // 日志器名（可选）
        Level level{};                                // 级别
        std::chrono::system_clock::time_point time{}; // 事件时间
        std::string payload;                          // 正文（已由 Logger 变参格式化而来）
    };
} // namespace bitlog
