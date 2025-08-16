#include "util.hpp"
#include "level.hpp"
int main()
{
    std::cout << mylog::LogLevel::toString(mylog::LogLevel::value::DEBUG) << std::endl;
    std::cout << mylog::LogLevel::toString(mylog::LogLevel::value::INFO) << std::endl;
    std::cout << mylog::LogLevel::toString(mylog::LogLevel::value::WARN) << std::endl;
    std::cout << mylog::LogLevel::toString(mylog::LogLevel::value::FATAL) << std::endl;
    std::cout << mylog::LogLevel::toString(mylog::LogLevel::value::OFF) << std::endl;
    
    std::cout << mylog::util::Date::now() << std::endl;
    std::string pathname = "./asd/sad/ww/a.txt";
    mylog::util::File::createDirectory(mylog::util::File::path(pathname));
    if (mylog::util::File::exists(mylog::util::File::path(pathname)))
    {
        std::cout << "存在，创建成功" << std::endl;
    }
    else
    {
        std::cout << "不存在，创建失败" << std::endl;
    }
}