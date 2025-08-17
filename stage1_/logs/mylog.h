#pragma once
#include "logger.hpp"
namespace mylog
{

// 1.提供获取指定日志器的全局接口（避免用户自己操作单例对象）
    Logger::ptr getLogger(const std::string &name)
    {
        return LoggerManager::getInstance().getLogger(name);
    }
    Logger::ptr rootLogger()
    {
        return LoggerManager::getInstance().rootLogger();
    }
// 2.使用宏函数，对日志器接口进行代理（代理模式）
#define debug(fmt, ...) debug(__FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define info(fmt, ...) info(__FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define warn(fmt, ...) warn(__FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define error(fmt, ...) error(__FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define fatal(fmt, ...) fatal(__FILE__, __LINE__, fmt, ##__VA_ARGS__)

#define LOG_DEBUG(logger, fmt, ...) (logger)->debug(fmt, ##__VA_ARGS__)
#define LOG_INFO(logger, fmt, ...) (logger)->info(fmt, ##__VA_ARGS__)
#define LOG_WARN(logger, fmt, ...) (logger)->warn(fmt, ##__VA_ARGS__)
#define LOG_ERROR(logger, fmt, ...) (logger)->error(fmt, ##__VA_ARGS__)
#define LOG_FATAL(logger, fmt, ...) (logger)->fatal(fmt, ##__VA_ARGS__)

// 3.提供宏函数，直接进行日志的标准输出打印（不用获取日志器）
#define LOGD(fmt, ...) LOG_DEBUG(mylog::rootLogger(), fmt, ##__VA_ARGS__)
#define LOGI(fmt, ...) LOG_INFO(mylog::rootLogger(), fmt, ##__VA_ARGS__)
#define LOGW(fmt, ...) LOG_WARN(mylog::rootLogger(), fmt, ##__VA_ARGS__)
#define LOGE(fmt, ...) LOG_ERROR(mylog::rootLogger(), fmt, ##__VA_ARGS__)
#define LOGF(fmt, ...) LOG_FATAL(mylog::rootLogger(), fmt, ##__VA_ARGS__)
}