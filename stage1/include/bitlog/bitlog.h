// include/bitlog/bitlog.h
// 概要：提供进程级 root() 与若干便捷宏。
// 用途：不显式创建 Logger 时可直接 LOGI/LOGE 输出；root 缺省接 StdoutSink。
#pragma once
#include "logger.hpp"
namespace bitlog
{
    Logger &root(); // 进程级单例（Meyers Singleton）
} // namespace bitlog

// 便捷宏：走 root() 的快捷等级接口
#define LOGD(fmt, ...) ::bitlog::root().debug(fmt, ##__VA_ARGS__)
#define LOGI(fmt, ...) ::bitlog::root().info(fmt, ##__VA_ARGS__)
#define LOGW(fmt, ...) ::bitlog::root().warn(fmt, ##__VA_ARGS__)
#define LOGE(fmt, ...) ::bitlog::root().error(fmt, ##__VA_ARGS__)
#define LOGF(fmt, ...) ::bitlog::root().fatal(fmt, ##__VA_ARGS__)
