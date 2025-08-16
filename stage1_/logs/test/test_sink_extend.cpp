#include "../util.hpp"
#include "../level.hpp"
#include "../message.hpp"
#include "../format.hpp"
#include "../sink.hpp"
#include "../sin_extend.hpp"

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
    std::string logger = "root";

    std::string file = "test_sink.cpp";
    LogMsg msg1;
    Formatter fmt;
    msg1.setPayload(readFileToString("./文本.txt"));

    // LogMsg msg(logger, file, 123, std::string("测试sink功能"), LogLevel::value::INFO);
    // LogSink::ptr stdout_lsp = SinkFactory<StdoutSink>::create();
    std::string basename = "./logfile";
    // LogSink::ptr file_lsp = SinkFactory<FileSink>::create(basename + "/file/test.log");
    LogSink::ptr roll_lsp = SinkFactory<RollBySizeSink>::create(basename + "/roll", 1024 * 1024);

    // std::string str = fmt.format(msg);
    // stdout_lsp->log(str.c_str(), str.size());
    // file_lsp->log(str.c_str(), str.size());
    // size_t cursize = 0;
    // size_t count = 0;
    // while (cursize < 1024)
    // {
    //     std::string tmp = std::to_string(++count) + str;
    //     roll_lsp->log(tmp.c_str(), tmp.size());
    //     cursize += tmp.size();
    // }
    std::string str1 = fmt.format(msg1);
    roll_lsp->log(str1.c_str(), str1.size());

    // extend
    LogSink::ptr time_lsp = SinkFactory<RollByTimeSink>::create(basename + "/time", TimeUnit::Secondly);
    int sec = 0;
    while (sec < 180)
    {
        time_lsp->log(str1.c_str(), str1.size());
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(1s);
        sec++;
    }
}
