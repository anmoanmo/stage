#pragma once

#include <iostream>
#include <string>
#include <thread>
#include "level.hpp"
#include "util.hpp"

namespace mylog
{
    struct LogMsg
    {

        LogMsg() : _ctime(util::Date::now()), _tid(std::this_thread::get_id()) {}

        LogMsg(std::string logger, std::string file, size_t line,
               std::string payload, LogLevel::value level = LogLevel::value::INFO)
            : _ctime(util::Date::now()), _line(line), _level(level), _tid(std::this_thread::get_id()), _file(std::move(file)), _logger(std::move(logger)), _payload(std::move(payload)) {}

        // ---------- getters ----------
        time_t getCtime() const noexcept { return _ctime; }
        size_t getLine() const noexcept { return _line; }
        LogLevel::value getLevel() const noexcept { return _level; }
        std::thread::id getTid() const noexcept { return _tid; }
        const std::string &getFile() const noexcept { return _file; }
        const std::string &getLogger() const noexcept { return _logger; }
        const std::string &getPayload() const noexcept { return _payload; }

        // ---------- setters（链式返回 *this） ----------
        LogMsg &setCtime(time_t t)
        {
            _ctime = t;
            return *this;
        }
        LogMsg &setLine(size_t l)
        {
            _line = l;
            return *this;
        }
        LogMsg &setLevel(LogLevel::value lv)
        {
            _level = lv;
            return *this;
        }
        LogMsg &setTid(std::thread::id tid)
        {
            _tid = tid;
            return *this;
        }
        LogMsg &setTidToCurrent()
        {
            _tid = std::this_thread::get_id();
            return *this;
        }

        LogMsg &setFile(std::string v)
        {
            _file = std::move(v);
            return *this;
        }
        LogMsg &setFile(const char *v)
        {
            _file = v;
            return *this;
        }

        LogMsg &setLogger(std::string v)
        {
            _logger = std::move(v);
            return *this;
        }
        LogMsg &setLogger(const char *v)
        {
            _logger = v;
            return *this;
        }

        LogMsg &setPayload(std::string v)
        {
            _payload = std::move(v);
            return *this;
        }
        LogMsg &setPayload(const char *v)
        {
            _payload = v;
            return *this;
        }

    private:
        time_t _ctime;          // 日志产生时间戳
        size_t _line;           // 错误发生行号
        LogLevel::value _level; // 日志等级
        std::thread::id _tid;   // 线程ID
        std::string _file;      // 错误产生的源码文件名
        std::string _logger;    // 日志器名称
        std::string _payload;   // 实际错误信息
    };
}

