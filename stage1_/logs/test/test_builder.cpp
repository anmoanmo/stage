#include "../util.hpp"
#include "../level.hpp"
#include "../message.hpp"
#include "../format.hpp"
#include "../sink.hpp"
#include "../logger.hpp"

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

std::string readFileToString(const std::string &filepath)
{
    std::ifstream ifs(filepath, std::ios::in | std::ios::binary);
    if (!ifs)
    {
        throw std::runtime_error("无法打开文件: " + filepath);
    }
    std::ostringstream oss;
    oss << ifs.rdbuf(); // 将文件流读入到字符串流
    return oss.str();
}

int main()
{
    using namespace mylog;

    std::string logger_name = "sync_log";
    LogLevel::value limit = LogLevel::value::DEBUG;
    std::vector<LogSink::ptr> sinks;
    sinks.push_back(SinkFactory<StdoutSink>::create());
    sinks.push_back(SinkFactory<FileSink>::create("./logfile/test_File.log"));
    sinks.push_back(SinkFactory<RollBySizeSink>::create("./logfile/test_roll", 1024 * 1024));
    Formatter::ptr fmt(new Formatter());

    std::unique_ptr<LoggerBuilder> builder(new LocalLoggerBuilder());
    builder->buildLoggerName(logger_name);
    builder->buildLoggerSink<StdoutSink>();
    builder->buildLoggerSink<FileSink>("./logfile/test_File.log");
    builder->buildLoggerSink<RollBySizeSink>("./logfile/test_roll", 1024 * 1024);
    Logger::ptr logger = builder->build();

    // Logger::ptr sync_ptr = std::make_shared<SyncLogger>(logger_name, limit, fmt, sinks);
    // Logger::ptr sync_ptr(new SyncLogger(logger_name, limit, fmt, sinks));
    size_t cursize = 0,
           count = 0;
    std::string msg = readFileToString("./short_text.txt");
    while (cursize < 1024 * 1024 * 10)
    {
        std::string tmp = std::to_string(++count) + msg;
        logger->info(__FILE__, __LINE__, "%s", msg.c_str());
        // sync_ptr->info(__FILE__, __LINE__, "%s", msg.c_str());
        //  传给 ... 的是 std::string，对应 %s 需要的是 const char*
        cursize += tmp.size();
    }
}