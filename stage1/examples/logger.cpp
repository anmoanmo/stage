// examples/logger.cpp
// 目标：验证最小链路是否能跑通；观察控制台输出一行 INFO 日志。
#include "bitlog/bitlog.h"
int main()
{
    auto &lg = bitlog::root();              // 取 root 记录器（stdout）
    lg.info("hello %s %d", "bitlog", 42);   // 成员方法调用
    LOGI("root macro %s=%d", "answer", 42); // 宏调用
    return 0;
}
