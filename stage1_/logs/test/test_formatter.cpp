// test_min_ok.cpp
#include "../format.hpp"
#include "../message.hpp"
#include "../level.hpp"
#include "../util.hpp"

#include <iostream>
#include <sstream>
#include <string>
#include <ctime>

int main()
{
    using namespace mylog;

    // 注意：不要写成 LogMsg msg(); —— 那是函数声明。
    std::string logger = "root"; // 第一个参数是 std::string&，必须是左值
    std::string file = "test_min_ok.cpp";

    // 你的 LogMsg 构造需要 5 个参数：(logger, file, line, payload, level)
    LogMsg msg(logger, file, 123, std::string("hello-formatter"), LogLevel::value::INFO);

    // 如需稳定输出，可固定时间戳（可选）
    msg.setCtime(1700000000);

    // 默认模式
    Formatter f_default;
    std::string out1 = f_default.format(msg);
    std::cout << "default : ⟦" << out1 << "⟧\n";

    // 自定义模式（覆盖 %d/%t/%c/%f:%l/%p/%T/%m/%n）
    const std::string pattern = "[%d{%Y-%m-%d %H:%M:%S}][%t][%c][%f:%l][%p]%T%m%";
    Formatter f_custom(pattern);
    std::string out2 = f_custom.format(msg);
    std::cout << "custom  : ⟦" << out2 << "⟧\n";

    // 转义 %% 与简单占位
    Formatter f_misc("HEAD%%TAIL %m%n");
    std::string out3 = f_misc.format(msg);
    std::cout << "misc    : ⟦" << out3 << "⟧\n";

    return 0;
}
