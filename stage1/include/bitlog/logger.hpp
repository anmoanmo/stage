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

namespace bitlog
{
    class Logger
    {
    public:
        using ptr = std::shared_ptr<Logger>;
        explicit Logger(std::string name);
        void set_level(Level lv) { _level.store(lv, std::memory_order_relaxed); }
        Level level() const { return _level.load(std::memory_order_relaxed); }
        void add_sink(LogSink::ptr s);
        void set_formatter(std::shared_ptr<Formatter> f);

        // 变参接口（C 风格，可与 printf 一致；后续替换为 {fmt} 可选）
        void debug(const char *fmt, ...);
        void info(const char *fmt, ...);
        void warn(const char *fmt, ...);
        void error(const char *fmt, ...);
        void fatal(const char *fmt, ...);

        const std::string &name() const { return _name; }

    private:
        void vlog(Level lv, const char *fmt, va_list ap); // 统一入口
        void log_it(const std::string &text);             // 串行写入 sinks
        static bool enabled(Level cur, Level req)
        {
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
    std::string vformat(const char *fmt, va_list ap);
} // namespace bitlog
