// src/bitlog.cpp
// 说明：Meyers Singleton。初始化阶段给 root 添加 StdoutSink 并设置默认等级。
#include "bitlog/bitlog.h"
namespace bitlog
{
    Logger &root()
    {
        static Logger logger("root"); // C++11 起线程安全
        static bool inited = []
        {
            logger.add_sink(std::make_shared<StdoutSink>());
            logger.set_level(Level::DEBUG);
            return true;
        }();
        (void)inited; // 防止未使用告警
        return logger;
    }
} // namespace bitlog
