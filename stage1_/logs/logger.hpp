/*日志器模块
    1.抽象日志器基类
    2.同步/异步日志器模块
*/
#pragma once

#include "level.hpp"
#include "message.hpp"
#include "util.hpp"
#include "format.hpp"
#include "sink.hpp"
#include "looper.hpp"
#include "buffer.hpp"

#include <atomic>
#include <mutex>
#include <sstream>
#include <cstdarg> // va_list, va_start, va_end
#include <cstdio>  // vasprintf（GNU 扩展，建议也包含）
#include <cstdlib> // free
#include <utility> // std::move
#include <memory>  // std::shared_ptr（如未包含）
#include <vector>  // std::vector（如未包含）
#include <atomic>
#include <string>
#include <unordered_map>
#include <cassert>

namespace mylog
{
    class Logger
    {
    public:
        using ptr = std::shared_ptr<Logger>;

        Logger(std::string name,                // 按值
               LogLevel::value level,           // 按值
               Formatter::ptr formatter,        // 按值（shared_ptr 拷一次或移动）
               std::vector<LogSink::ptr> sinks) // 按值（vector 拷一次或移动）
            : _logger_name(std::move(name)),
              _limit_level(level),
              _formatter(std::move(formatter)),
              _sinks(std::move(sinks))
        {
        } // 成员里统一 move

        virtual ~Logger() = default;

        /*
        构造日志消息对象过程并进行格式化，得到格式化后的日志消息字符串--等待进行落地输出
        通过传入的参数构造出一个日志消息对象，进行日志的格式化，最后落地
            1.判断当前日志消息是否达到输出等级
            2.对fmt格式化字符串和不定参数进行字符串组织，得到日志消息字符串
            3.构造LogMsg对象
            4.通过格式化工具对LogMsg进行格式化，得到格式化后的字符串
            5.进行日志落地
        */

        void debug(const std::string &file, size_t line,
                   const std::string fmt, ...)
        {
            // 2.
            va_list ap;
            va_start(ap, fmt);
            common_level(LogLevel::value::DEBUG, file, line, ap, fmt);
            va_end(ap);
        }

        void info(const std::string &file, size_t line,
                  const std::string fmt, ...)
        {
            va_list ap;
            va_start(ap, fmt);
            common_level(LogLevel::value::INFO, file, line, ap, fmt);
            va_end(ap);
        }

        void warn(const std::string &file, size_t line,
                  const std::string fmt, ...)
        {
            va_list ap;
            va_start(ap, fmt);
            common_level(LogLevel::value::WARN, file, line, ap, fmt);
            va_end(ap);
        }

        void error(const std::string &file, size_t line,
                   const std::string fmt, ...)
        {
            va_list ap;
            va_start(ap, fmt);
            common_level(LogLevel::value::ERROR, file, line, ap, fmt);
            va_end(ap);
        }

        void fatal(const std::string &file, size_t line,
                   const std::string fmt, ...)
        {
            va_list ap;
            va_start(ap, fmt);
            common_level(LogLevel::value::FATAL, file, line, ap, fmt);
            va_end(ap);
        }

        virtual void setMaxBufferSize(size_t max_size) {}

    private: //(protected)
        void common_level(const LogLevel::value level,
                          const std::string &file,
                          const size_t line,
                          va_list ap,
                          const std::string fmt)
        {
            // std::atomic<T> 没有隐式转换，依赖实现可能不编；规范写法要 load()。
            if (level < _limit_level.load(std::memory_order_relaxed))
            {
                return;
            }

            char *res;
            int ret = vasprintf(&res, fmt.c_str(), ap);
            assert(ret != -1);
            LogMsg msg(_logger_name, file, line, res, level);
            free(res);
            std::stringstream ss;
            _formatter->format(ss, msg);
            std::string str = ss.str();
            log(str.c_str(), str.size());
        }
        virtual void log(const char *data, size_t len) {}

    protected:
        std::mutex _mutex;
        std::string _logger_name;
        std::atomic<LogLevel::value> _limit_level; // 原子化元素，避免高频访问带来的性能降低
        Formatter::ptr _formatter;
        std::vector<LogSink::ptr> _sinks;
    };

    class SyncLogger : public Logger
    {
    public:
        SyncLogger(std::string name,         // 按值
                   LogLevel::value level,    // 按值
                   Formatter::ptr formatter, // 按值（shared_ptr 拷一次或移动）
                   std::vector<LogSink::ptr> sinks)
            : Logger(name, level, formatter, sinks) {};

    private:
        // 同步日志器，将日志直接通过落地模块进行日志落地
        virtual void log(const char *data, size_t len) override
        {
            std::unique_lock<std::mutex> lock(_mutex);
            if (_sinks.empty())
            {
                return;
            }
            for (auto &sink : _sinks)
            {
                sink->log(data, len);
            }
        }
    };

    class AsyncLogger : public Logger
    {
    public:
        AsyncLogger(std::string name,         // 按值
                    LogLevel::value level,    // 按值
                    Formatter::ptr formatter, // 按值（shared_ptr 拷一次或移动）
                    std::vector<LogSink::ptr> sinks)
            : Logger(name, level, formatter, sinks),
              _looper(std::make_shared<AsyncLooper>(std::bind(&AsyncLogger::realLog, this, std::placeholders::_1))) {};

        virtual void log(const char *data, size_t len) override
        {
            _looper->push(data, len);
        };
        void realLog(Buffer &buf)
        {
            if (_sinks.empty())
            {
                return;
            }

            const char *p = buf.readPtr();
            const char *end = p + buf.readableSize();
            const char *line = p;

            while (line < end)
            {
                // 找下一条换行（Windows 下行尾是 "\r\n"，找 '\n' 也没问题）
                const char *nl = static_cast<const char *>(
                    ::memchr(line, '\n', static_cast<size_t>(end - line)));

                size_t len = nl ? static_cast<size_t>(nl - line + 1)
                                : static_cast<size_t>(end - line); // 最后一条可能没 '\n'

                for (auto &sink : _sinks)
                    sink->log(line, len);

                if (!nl)
                {
                    break; // 没有更多完整行
                }
                line = nl + 1;
            }
        };

        virtual void setMaxBufferSize(size_t max_size) override
        {
            if (_looper)
            {
                _looper->setMaxBufferSize(max_size);
            }
        }

        // void setAsyncBufferGrowth(size_t threshold, size_t increment)
        // {
        //     _async_threshold = threshold;
        //     _async_increment = increment;
        // }

    private:
        AsyncLooper::ptr _looper;
    };

    /*
    使用建造者模式来建造日志器，简化使用复杂度
    */
    // 1.抽象一个建造者类（完成所有日志器对象所需零件的构建 & 日志器的构建）
    //      1.设置日志器类型
    //      2. 将不同类型日志器的创建放在同一个日志器建造者类中完成
    // 2.派生出具体的建造着类----局部日志器的建造者&&全局日志器的建造者（添加全局单例管理器，将日志器添加到全局管理器中）

    enum class LoggerType
    {
        LOGGER_SYNC,
        LOGGER_ASYNC
    };

    class LoggerBuilder
    {
    public:
        using ptr = std::shared_ptr<LoggerBuilder>;
        LoggerBuilder() : _logger_type(LoggerType::LOGGER_SYNC),
                          _limit_value(LogLevel::value::DEBUG) {}

        void buildLoggerType(LoggerType type)
        {
            /*默认同步*/
            _logger_type = type;
        };
        void buildLoggerName(const std::string &name)
        {
            /*必须自定义*/
            _logger_name = name;
        };
        void buildLoggerLevel(LogLevel::value level)
        {
            /*默认DEBUG*/
            _limit_value = level;
        };
        void buildLoggerFormatter(const std::string &pattern)
        {
            /*默认常规格式 "[%d{%H:%M:%S}][%t][%c][%f:%l][%p]%T%m%n"*/
            _formatter = std::make_shared<Formatter>(pattern);
        };

        template <typename SinkType, typename... Args>
        void buildLoggerSink(Args &&...args)
        {
            /*默认标准输出*/
            LogSink::ptr psink = SinkFactory<SinkType>::create(std::forward<Args>(args)...);
            _sinks.push_back(psink);
        }

        void buildAsyncBufferMax(size_t max_bytes)
        {
            _async_max_buf = max_bytes;
        }
        // void buildAsyncBufferGrowth(size_t threshold, size_t increment)
        // {
        //     _async_threshold = threshold;
        //     _async_increment = increment;
        // }

        virtual Logger::ptr build() = 0;

    protected:
        LoggerType _logger_type;
        std::string _logger_name;
        std::atomic<LogLevel::value> _limit_value;
        Formatter::ptr _formatter;
        std::vector<LogSink::ptr> _sinks;

        size_t _async_max_buf = 200 * 1024 * 1024;
        // size_t _async_threshold = THRESHOLD_BUFFER_SIZE; // 可选：若你愿意开放
        // size_t _async_increment = INCREMENT_BUFFER_SIZE; // 可选：若你愿意开放
    };

    class LocalLoggerBuilder : public LoggerBuilder
    {
    public:
        virtual Logger::ptr build() override
        {
            assert(!_logger_name.empty());
            if (!_formatter.get())
            {
                _formatter = std::make_shared<Formatter>();
            }
            if (_sinks.empty())
            {
                // LogSink::ptr psink = SinkFactory<StdoutSink>::create();
                // _sinks.push_back(psink);
                buildLoggerSink<StdoutSink>();
            }

            if (_logger_type == LoggerType::LOGGER_SYNC)
            {
                return std::make_shared<SyncLogger>(_logger_name,
                                                    _limit_value,
                                                    _formatter,
                                                    _sinks);
            }
            else
            {
                auto logger = std::make_shared<AsyncLogger>(_logger_name,
                                                            _limit_value,
                                                            _formatter,
                                                            _sinks);
                logger->setMaxBufferSize(_async_max_buf);
                // 如果你想进一步开放阈值/增量（需要在 AsyncLooper/Buffer 暴露对应方法）
                // logger->setBufferGrowth(_async_threshold, _async_increment);
                return logger;
            }
        };
    };

    class LoggerManager
    {
    private:
        std::mutex _mutex;
        Logger::ptr _root_logger;
        std::unordered_map<std::string, Logger::ptr> _loggers;

    private:
        LoggerManager()
        {
            // std::unique_ptr<LocalLoggerBuilder> slb(new LocalLoggerBuilder());
            // slb->buildLoggerName("root");
            // slb->buildLoggerType(LoggerType::LOGGER_SYNC);
            // _root_logger = slb->build();
            // _loggers.insert(std::make_pair("root", _root_logger));

            /*你在 loggerManager() 构造函数里用 LocalLoggerBuilder 造了 root（同步），
            顺序上没问题（LocalLoggerBuilder 在前）。
            但这会把 manager 与 builder硬耦合。如果将来你再拆分模块，很容易又回到“声明不可见”的老问题。最小更稳的做法：
            直接手工构建 root（不通过 builder），语义不变：*/
            Formatter::ptr fmt = std::make_shared<Formatter>();
            std::vector<LogSink::ptr> sinks;
            sinks.push_back(std::make_shared<StdoutSink>());
            _root_logger = std::make_shared<SyncLogger>("root", LogLevel::value::DEBUG, fmt, sinks);
            _loggers.emplace("root", _root_logger);
        }
        LoggerManager(const LoggerManager &) = delete;
        LoggerManager &operator=(const LoggerManager &) = delete;

    public:
        static LoggerManager &getInstance()
        {
            static LoggerManager lm;
            return lm;
        }
        bool hasLogger(const std::string &name)
        {
            std::unique_lock<std::mutex> lock(_mutex);
            auto it = _loggers.find(name);
            if (it == _loggers.end())
            {
                return false;
            }
            return true;
        }
        void addLogger(const std::string &name, const Logger::ptr &logger)
        {
            std::unique_lock<std::mutex> lock(_mutex);
            if (_loggers.find(name) != _loggers.end())
                return;
            _loggers.emplace(name, logger);
        }
        Logger::ptr getLogger(const std::string &name)
        {
            std::unique_lock<std::mutex> lock(_mutex);
            auto it = _loggers.find(name);
            if (it == _loggers.end())
            {
                return Logger::ptr();
            }
            return it->second;
        }
        Logger::ptr rootLogger()
        {
            std::unique_lock<std::mutex> lock(_mutex);
            return _root_logger;
        }
    };

    class GlobalLoggerBuilder : public LoggerBuilder
    {
    public:
        virtual Logger::ptr build() override
        {
            if (_logger_name.empty())
            {
                std::cout << "日志器名称不能为空！！";
                abort();
            }
            if (LoggerManager::getInstance().hasLogger(_logger_name))
            {
                // 与管理器对齐，避免返回未注册实例
                return LoggerManager::getInstance().getLogger(_logger_name);
            }
            assert(LoggerManager::getInstance().hasLogger(_logger_name) == false);
            if (_formatter.get() == nullptr)
            {
                std::cout << "当前日志器：" << _logger_name << " 未检测到日志格式，默认设置为[ %d{%H:%M:%S}%T%t%T[%p]%T[%c]%T%f:%l%T%m%n ]!\n";
                _formatter = std::make_shared<Formatter>();
            }
            if (_sinks.empty())
            {
                std::cout << "当前日志器：" << _logger_name << " 未检测到落地方向，默认设置为标准输出!\n";
                buildLoggerSink<StdoutSink>();
            }
            Logger::ptr lp;
            if (_logger_type == LoggerType::LOGGER_ASYNC)
            {
                lp = std::make_shared<AsyncLogger>(_logger_name, _limit_value, _formatter, _sinks);
                lp->setMaxBufferSize(_async_max_buf); // ← 补上这一行
            }
            else
            {
                lp = std::make_shared<SyncLogger>(_logger_name, _limit_value, _formatter, _sinks);
            }

            LoggerManager::getInstance().addLogger(_logger_name, lp);
            return lp;
        }
    };
}