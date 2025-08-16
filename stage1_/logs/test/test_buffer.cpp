#include "../buffer.hpp"

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
    // 读取文件数据，一点点写入缓冲区，最终将缓冲区数据写入文件，判断生成的新文件与源文件是否一致
    std::ifstream ifs("long_text.txt", std::ios::binary);
    if (!ifs.is_open())
    {
        std::cerr << "ifs打开失败" << std::endl;
        return 0;
    }
    // ifs.seekg(0, std::ios::end);//跳转到文件末尾
    // size_t fsize = ifs.tellg();//获取当前读写位置相较于起始位置的偏移量
    // ifs.seekg(0, std::ios::beg);
    // std::string body;
    // ifs.read(&body[0], fsize);
    std::ostringstream oss;
    oss << ifs.rdbuf();
    ifs.close();
    std::string str = oss.str();

    Buffer buffer;
    bool ok = buffer.push(str.data(), str.size()); // push 已经推进写指针
    if (!ok)
    {
        std::cerr << "buffer 不够大\n";
    }
    std::ofstream ofs("./logfile/tmp.log", std::ios::out | std::ios::binary);
    //ofs.write(buffer.readPtr(), buffer.readableSize());
    for (int i = 0; i < buffer.readableSize(); i++)
    {
        ofs.write(buffer.readPtr(), 1);
        buffer.moveReader(1);
    }
    ofs.close();
}