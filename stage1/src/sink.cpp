// src/sink.cpp
// 说明：使用 cout.write 避免额外格式化；立即 flush 方便调试观察。
#include <iostream>
#include "bitlog/sink.hpp"
namespace bitlog
{
    void StdoutSink::log(const std::string &text)
    {
        std::cout.write(text.data(), static_cast<std::streamsize>(text.size()));
        std::cout.flush();
    }
} // namespace bitlog
