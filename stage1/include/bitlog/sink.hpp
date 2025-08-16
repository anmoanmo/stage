// include/bitlog/sink.hpp
// 概要：抽象落地器接口 + 标准输出实现。
// 用途：后端可以替换为文件、滚动文件、网络等；本步仅提供 StdoutSink。
#pragma once
#include <memory>
#include <string>
namespace bitlog
{
    struct LogSink
    {
        using ptr = std::shared_ptr<LogSink>;
        virtual ~LogSink() = default;
        // 约定：接收“完整的一行字符串”（含或不含换行均可，具体由 Formatter 决定）
        virtual void log(const std::string &text) = 0;
    };
    struct StdoutSink final : LogSink
    {
        void log(const std::string &text) override;
    };
} // namespace bitlog
