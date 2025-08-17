#include "logs/logger.hpp"

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

    std::string logger_name = "async_log";
    // LogLevel::value limit = LogLevel::value::DEBUG;
    // std::vector<LogSink::ptr> sinks;
    // sinks.push_back(SinkFactory<StdoutSink>::create());
    // sinks.push_back(SinkFactory<FileSink>::create("./logfile/test_File.log"));
    // sinks.push_back(SinkFactory<RollBySizeSink>::create("./logfile/test_roll", 1024 * 1024));
    // Formatter::ptr fmt(new Formatter());

    std::unique_ptr<LoggerBuilder> builder(new LocalLoggerBuilder());
    builder->buildLoggerType(LoggerType::LOGGER_ASYNC);
    builder->buildLoggerName(logger_name);
    builder->buildLoggerSink<StdoutSink>();
    builder->buildLoggerSink<FileSink>("./logfile/test_File_async.log");
    builder->buildLoggerSink<RollBySizeSink>("./logfile/test_roll__async", 1000);
    builder->buildAsyncBufferMax(1024*1024*200);

    Logger::ptr logger = builder->build();

    // Logger::ptr sync_ptr = std::make_shared<SyncLogger>(logger_name, limit, fmt, sinks);
    // Logger::ptr sync_ptr(new SyncLogger(logger_name, limit, fmt, sinks));
    size_t cursize = 0,
           count = 0;
    std::string msg = readFileToString("./short_text.txt");
    while (count < 100)
    {
        logger->info(__FILE__, __LINE__, "%s+%zu", msg.c_str(), ++count);
        // sync_ptr->info(__FILE__, __LINE__, "%s", msg.c_str());
        //  传给 ... 的是 std::string，对应 %s 需要的是 const char*
        cursize += msg.size();
    }
}