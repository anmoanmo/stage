/*日志落地模块的实现
    1.抽象落地基类
    2.派生子类（根据不同落地方向进行派生）
    3.使用工厂模式进行创建与表示的分离
*/
#pragma once

#include <unistd.h>
#include <memory>
#include <iostream>
#include <sstream>
#include <fstream>
#include <cassert>
#include <string>

#include "format.hpp"
#include "util.hpp"
#include "message.hpp"
#include "level.hpp"

namespace mylog
{
    class LogSink
    {
    public:
        using ptr = std::shared_ptr<LogSink>;
        LogSink() {}
        virtual ~LogSink() {}
        virtual void log(const char *data, size_t len) = 0;
    };

    // 落地方向：标准输出
    class StdoutSink : public LogSink
    {
    public:
        virtual void log(const char *data, size_t len) override
        {
            std::cout.write(data, len);
        }
    };
    // 落地方向：指定文件
    class FileSink : public LogSink
    {
    public:
        // 构造是传入文件名，并打开文件，将操作句柄进行管理
        FileSink(const std::string &pathname) : _pathname(pathname)
        {
            // 1. 创建日志文件所在的目录
            if (!util::File::exists(util::File::path(pathname)))
            {
                util::File::createDirectory(util::File::path(pathname));
            }
            // 2.创建并打开日志文件
            _ofs.open(_pathname, std::ios::binary | std::ios::app);
            assert(_ofs.is_open());
        }
        // 将日志消息进行写入
        virtual void log(const char *data, size_t len) override
        {
            _ofs.write(data, len);
            assert(_ofs.good());
        }

    private:
        std::string _pathname;
        std::ofstream _ofs;
    };

    // 落地方向：滚动文件（以大小进行滚动）
    class RollBySizeSink : public LogSink
    {
    public:
        RollBySizeSink(const std::string &basename, size_t max_size) : _basename(basename),
                                                                       _max_size(max_size), _cur_size(0), _seq(0)
        {
            // // 1. 创建日志文件所在的目录
            // std::string pathname = createNewFile();
            // if (!util::File::exists(util::File::path(pathname)))
            // {
            //     util::File::createDirectory(util::File::path(pathname));
            // }
            // // 2.创建并打开日志文件
            // _ofs.open(pathname, std::ios::binary | std::ios::app);
            // assert(_ofs.is_open());
            // _cur_size = static_cast<size_t>(_ofs.tellp());
        }

        // 将日志消息进行写入
        virtual void log(const char *data, size_t len) override
        {
            /*
            在异步场景下，realLog() 一次写进来的 len 可能很大（缓冲里积累了很多条日志）。如果当前 _cur_size 还没到 10 字节（比如 0），就不会 rotate，结果先把一大坨一次性写进去，第一份文件直接超限，后面才可能开始滚动。
            */
            // if (_cur_size + len > _max_size)
            // { // 预判
            //     rotate();
            // }
            // _ofs.write(data, len);
            // assert(_ofs.good());
            // _cur_size += len;
            if (!_ofs.is_open())
            {
                const std::string pathname = createNewFile();
                if (!util::File::exists(util::File::path(pathname)))
                    util::File::createDirectory(util::File::path(pathname));
                _ofs.open(pathname, std::ios::binary | std::ios::app);
                assert(_ofs.is_open());
                _cur_size = 0; // 首开一定从 0 开始
            }

            if (_cur_size + len > _max_size && _cur_size > 0)
            {
                rotate();
            }
            _ofs.write(data, len);
            if (!_ofs.good())
                std::cerr << "RollBySizeSink write error\n";
            _cur_size += len;
            if (_cur_size >= _max_size)
            {
                rotate();
            }
        }

    private:
        void rotate()
        {
            _ofs.flush();
            _ofs.close();
            const std::string pathname = createNewFile();
            if (!util::File::exists(util::File::path(pathname)))
            {
                util::File::createDirectory(util::File::path(pathname));
            }
            _ofs.open(pathname, std::ios::binary | std::ios::app);
            assert(_ofs.is_open());
            _cur_size = static_cast<size_t>(_ofs.tellp());
        }

        std::string createNewFile()
        {
            time_t t = util::Date::now();
            struct tm lt{};
            localtime_r(&t, &lt);
            std::stringstream ss;
            if (t != _last_sec)
            {
                _seq = 0;
                _last_sec = t;
            }

            // 年月日时分秒修正 + 加序号避免同秒冲突
            ss << _basename
               << "_" << (lt.tm_year + 1900)
               << "-" << (lt.tm_mon + 1)
               << "-" << lt.tm_mday
               << "-" << lt.tm_hour
               << "-" << lt.tm_min
               << "-" << lt.tm_sec
               << "_" << _seq++
               << ".log";
            return ss.str();
        }

    private:
        // 在基础文件名上进行追加扩展（文件名 = basename + addname）
        std::string _basename;
        std::ofstream _ofs;
        size_t _max_size;
        size_t _cur_size;
        size_t _seq;
        time_t _last_sec{0};
    };

    template <typename SinkType>
    class SinkFactory
    {
    public:
        template <typename... Args>
        static LogSink::ptr create(Args &&...args)
        {
            return std::make_shared<SinkType>(std::forward<Args>(args)...);
        }
    };
};