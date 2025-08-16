/*
    通用功能类，与业务无关的功能实现
        1. 获取系统时间
        2. 获取文件大小
        3. 创建目录
        4. 获取文件所在目录
*/
#pragma once

#include <string>
#include <iostream>
#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace mylog
{
    namespace util
    {
        class Date
        {
        public:
            static size_t now()
            {
                return (size_t)time(nullptr);
            }
        };

        class File
        {
        public:
            static bool exists(const std::string &pathname)
            {
                struct stat st;
                if (stat(pathname.c_str(), &st) < 0)
                {
                    return false;
                }
                return true;
                // return (access(pathname.c_str(), F_OK) == 0); // 具有os局限性
                // //
            };
            static std::string path(const std::string &pathname)
            {
                size_t pos = pathname.find_last_of("/\\");
                if (pos == std::string::npos)
                {
                    return ".";
                }
                return pathname.substr(0, pos + 1);
            };

            static void createDirectory(const std::string &pathname)
            {
                // 统一分隔符到 '/'
                std::string path = pathname;
                for (auto &c : path)
                    if (c == '\\')
                        c = '/';

                size_t idx = 0;
                while (idx < path.size())
                {
                    size_t pos = path.find('/', idx);
                    size_t end = (pos == std::string::npos) ? path.size() : pos;

                    // 累计前缀 [0, end)
                    std::string prefix = path.substr(0, end);

                    // 跳过空段 / "." / ".."
                    if (!prefix.empty() && prefix != "." && prefix != "..")
                    {
                        if (::mkdir(prefix.c_str(), 0755) == -1 && errno != EEXIST)
                        {
                            throw std::runtime_error(
                                std::string("mkdir failed: ") + prefix + " : " + std::strerror(errno));
                        }
                    }

                    if (pos == std::string::npos)
                        break; // 已处理到整条路径
                    idx = pos + 1;

                    // 合并多重分隔符
                    while (idx < path.size() && path[idx] == '/')
                        ++idx;
                }
            }
        };
    };
}

// #include <filesystem>
// #include <system_error>

// void ensureParentDirectories(const std::string &filePath)
// {
//     namespace fs = std::filesystem;
//     fs::path p(filePath);
//     fs::path dir = p.has_filename() ? p.parent_path() : p; // 如果传的是目录本身也可用
//     if (dir.empty())
//         return;

//     std::error_code ec;
//     fs::create_directories(dir, ec); // 等价于 mkdir -p
//     if (ec)
//     {
//         throw std::system_error(ec, "create_directories failed: " + dir.string());
//     }
// }
