#pragma once

#include "util.hpp"
#include "level.hpp"
#include "message.hpp"
#include "format.hpp"
#include "sink.hpp"

#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <filesystem>
#include <chrono>
#include <thread>
#include <type_traits>

/*
扩展一个以时间段为滚动条件的sink落地
*/
namespace mylog
{
    // 时间粒度
    enum class TimeUnit
    {
        Secondly,
        Minutely,
        Hourly,
        Daily
    };

    class RollByTimeSink : public LogSink
    {
    public:
        RollByTimeSink(const std::string &basename, TimeUnit unit)
            : _basename(basename), _unit(unit), _seq(0)
        {
            // 初始化当前时间段（bucket）
            time_t now = util::Date::now();
            _bucket_start = floorToUnit(now, _unit);
            _next_cut = nextCutFrom(_bucket_start, _unit);

            // 1. 生成文件名
            std::string pathname = createNewFile(_bucket_start);
            // 2. 确保目录存在
            const std::string parent = util::File::path(pathname);
            if (!parent.empty() && !util::File::exists(parent))
                util::File::createDirectory(parent);
            // 3. 打开文件
            _ofs.open(pathname, std::ios::binary | std::ios::app);
            assert(_ofs.is_open());
        }

        // 将日志消息进行写入（跨时间段则滚动）
        void log(const char *data, size_t len) override
        {
            time_t now = util::Date::now();
            if (now >= _next_cut)
            {
                rotate(now);
            }
            _ofs.write(data, static_cast<std::streamsize>(len));
            assert(_ofs.good());
        }

    private:
        void rotate(time_t now)
        {
            _ofs.close();

            // 进入新时间段，序号重置
            _bucket_start = floorToUnit(now, _unit);
            _next_cut = nextCutFrom(_bucket_start, _unit);
            _seq = 0;

            const std::string pathname = createNewFile(_bucket_start);
            const std::string parent = util::File::path(pathname);
            if (!parent.empty() && !util::File::exists(parent))
                util::File::createDirectory(parent);
            _ofs.open(pathname, std::ios::binary | std::ios::app);
            assert(_ofs.is_open());
        }

        // 将时间对齐到时间段起点（本地时间）
        static time_t floorToUnit(time_t t, TimeUnit unit)
        {
            struct tm lt{};
            localtime_r(&t, &lt);
            switch (unit)
            {
            case TimeUnit::Secondly:
                break; // 不改秒
            case TimeUnit::Minutely:
                lt.tm_sec = 0;
                break;
            case TimeUnit::Hourly:
                lt.tm_min = 0;
                lt.tm_sec = 0;
                break;
            case TimeUnit::Daily:
                lt.tm_hour = 0;
                lt.tm_min = 0;
                lt.tm_sec = 0;
                break;
            }
            lt.tm_isdst = -1;
            return std::mktime(&lt);
        }

        // 计算下一次切割时间点
        static time_t nextCutFrom(time_t bucket_start, TimeUnit unit)
        {
            switch (unit)
            {
            case TimeUnit::Secondly:
                return bucket_start + 1;
            case TimeUnit::Minutely:
                return bucket_start + 60;
            case TimeUnit::Hourly:
                return bucket_start + 60 * 60;
            case TimeUnit::Daily:
                return bucket_start + 24 * 60 * 60;
            }
            return bucket_start + 60;
        }

        // 生成唯一文件名：basename_YYYY.MM.DD_HH:MM:SS_seq.log
        // 若同名存在则递增 _seq 直到唯一；同一时间段内 _seq 递增，跨时间段重置为 0
        std::string createNewFile(time_t bucket_start)
        {
            struct tm lt{};
            localtime_r(&bucket_start, &lt);

            while (true)
            {
                std::stringstream ss;
                ss << _basename
                   << "_" << (lt.tm_year + 1900)
                   << "." << (lt.tm_mon + 1)
                   << "." << lt.tm_mday
                   << "_" << lt.tm_hour
                   << ":" << lt.tm_min
                   << ":" << lt.tm_sec
                   << "_" << _seq
                   << ".log";
                std::string path = ss.str();

                if (!util::File::exists(path))
                {
                    ++_seq; // 预留下一个序号
                    return path;
                }
                ++_seq; // 已存在则尝试下一个序号
            }
        }

    private:
        std::string _basename;
        std::ofstream _ofs;
        TimeUnit _unit;

        time_t _bucket_start{0}; // 当前时间段起点
        time_t _next_cut{0};     // 下一次切割时间
        size_t _seq;             // 当前时间段内的序号（进入新段重置为 0）
    };
}