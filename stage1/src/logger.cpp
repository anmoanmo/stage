// src/logger.cpp
#include <vector>
#include <cassert>
#include <cstdio>
#include "bitlog/logger.hpp"

namespace bitlog
{

    Logger::Logger(std::string name)
        : _name(std::move(name)),
          _formatter(std::make_shared<Formatter>())
    {
        _sinks.reserve(1); // 小优化：默认至少一个 sink
    }

    void Logger::add_sink(LogSink::ptr s) { _sinks.push_back(std::move(s)); }
    void Logger::set_formatter(std::shared_ptr<Formatter> f) { _formatter = std::move(f); }

    // 五个等级的变参接口：统一走 vlog
    void Logger::debug(const char *fmt, ...)
    {
        va_list ap;
        va_start(ap, fmt);
        vlog(Level::DEBUG, fmt, ap);
        va_end(ap);
    }
    void Logger::info(const char *fmt, ...)
    {
        va_list ap;
        va_start(ap, fmt);
        vlog(Level::INFO, fmt, ap);
        va_end(ap);
    }
    void Logger::warn(const char *fmt, ...)
    {
        va_list ap;
        va_start(ap, fmt);
        vlog(Level::WARN, fmt, ap);
        va_end(ap);
    }
    void Logger::error(const char *fmt, ...)
    {
        va_list ap;
        va_start(ap, fmt);
        vlog(Level::ERROR, fmt, ap);
        va_end(ap);
    }
    void Logger::fatal(const char *fmt, ...)
    {
        va_list ap;
        va_start(ap, fmt);
        vlog(Level::FATAL, fmt, ap);
        va_end(ap);
    }

    void Logger::vlog(Level lv, const char *fmt, va_list ap)
    {
        if (!enabled(_level.load(std::memory_order_relaxed), lv))
            return;

        // 1) 先用 vsnprintf 生成 payload（C 风格，printf 兼容）
        std::string payload = vformat(fmt, ap);

        // 2) 组装结构化消息
        LogMsg msg;
        msg.name = _name;
        msg.level = lv;
        msg.time = std::chrono::system_clock::now();
        msg.payload = std::move(payload);

        // 3) Formatter 输出最终文本
        std::string text = _formatter->format(msg);

        // 4) 串行写入所有 Sink
        log_it(text);
    }

    void Logger::log_it(const std::string &text)
    {
        std::lock_guard<std::mutex> lk(_mutex); // 实现相关：同一 Logger 内部串行
        for (auto &s : _sinks)
            s->log(text);
    }

    // vsnprintf 两段式：先求长度，再一次性填充
    // 注意：这是实现相关行为；当格式串与实参类型不匹配时，行为未定义（与 printf 一致）。
    std::string vformat(const char *fmt, va_list ap)
    {
        va_list ap2;
        va_copy(ap2, ap);
        const int n = std::vsnprintf(nullptr, 0, fmt, ap2); // 计算需要的字节数（不含 '\0'）
        va_end(ap2);
        if (n <= 0)
            return std::string();                      // 防御：异常或空
        std::string buf(static_cast<size_t>(n), '\0'); // 精确分配
        std::vsnprintf(buf.data(), static_cast<size_t>(n) + 1, fmt, ap);
        return buf; // 注意：buf 不含终止符
    }
} // namespace bitlog
