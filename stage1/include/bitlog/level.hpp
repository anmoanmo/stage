// include/bitlog/level.hpp
// 概要：日志等级枚举与字符串化。
// 用途：控制是否输出；格式化时打印等级文本。
// 语义：等级越高表示越严重；OFF 表示关闭输出。
// 注意：to_string 对未知值返回 "UNKNOW"，不要依赖枚举的具体整数值做跨模块协议。

#pragma once
namespace bitlog
{
    enum class Level : int
    {
        DEBUG = 0,
        INFO,
        WARN,
        ERROR,
        FATAL,
        OFF,
    };

    /*
    inline 允许在多个翻译单元中出现相同定义，并且在链接阶段合并为一个实体，
    从而避免 ODR 冲突。它不保证编译过程中只有一份副本，
    也不保证机器码中一定只保留一份实现。
    */
    inline const char *to_string(Level lv)
    {
        switch (lv)
        {
        case Level::DEBUG:
            return "DEBUG";
        case Level::INFO:
            return "INFO";
        case Level::WARN:
            return "WARN";
        case Level::ERROR:
            return "ERROR";
        case Level::FATAL:
            return "FATAL";
        case Level::OFF:
            return "OFF";
        default:
            return "UNKNOW"; // 实现相关：防御性返回
        }
    }
} // namespace bitlog
