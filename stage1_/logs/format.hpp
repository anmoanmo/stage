#pragma once

#include "util.hpp"
#include "message.hpp"
#include "level.hpp"
#include <memory>
#include <cassert>
#include <vector>
#include <tuple>
#include <sstream>

namespace mylog
{
    class FormatItem
    {
    public:
        using ptr = std::shared_ptr<FormatItem>;
        virtual void format(std::ostream &out, const LogMsg &msg) = 0;
    };

    // 子项
    class MsgFormatItem : public FormatItem
    {
    public:
        virtual void format(std::ostream &out, const LogMsg &msg) override
        {
            out << msg.getPayload();
        }
    };

    class LevelFormatItem : public FormatItem
    {
    public:
        virtual void format(std::ostream &out, const LogMsg &msg) override
        {
            out << mylog::LogLevel::toString(msg.getLevel());
        }
    };

    class TimeFormatItem : public FormatItem
    {
    public:
        TimeFormatItem(const std::string &fmt = "%H:%M:%S") : _time_fmt(fmt) {}
        virtual void format(std::ostream &out, const LogMsg &msg) override
        {
            struct tm t;
            time_t ts = msg.getCtime();
            localtime_r(&ts, &t);
            char tmp[64] = {0};
            strftime(tmp, 31, _time_fmt.c_str(), &t);
            out << tmp;
        }

    private:
        std::string _time_fmt; //%H:%M:%S
    };

    class FileFormatItem : public FormatItem
    {
    public:
        virtual void format(std::ostream &out, const LogMsg &msg) override
        {
            out << msg.getFile();
        }
    };

    class LineFormatItem : public FormatItem
    {
    public:
        virtual void format(std::ostream &out, const LogMsg &msg) override
        {
            out << msg.getLine();
        }
    };

    class ThreadFormatItem : public FormatItem
    {
    public:
        virtual void format(std::ostream &out, const LogMsg &msg) override
        {
            out << msg.getTid();
        }
    };

    class LoggerFormatItem : public FormatItem
    {
    public:
        virtual void format(std::ostream &out, const LogMsg &msg) override
        {
            out << msg.getLogger();
        }
    };

    class NLineFormatItem : public FormatItem
    {
    public:
        virtual void format(std::ostream &out, const LogMsg &msg) override
        {
            out << "\r\n";
        }
    };

    class OtherFormatItem : public FormatItem
    {
    public:
        OtherFormatItem(const std::string &str) : _str(std::move(str)) {}
        virtual void format(std::ostream &out, const LogMsg &msg) override
        {
            out << _str;
        }

    private:
        std::string _str;
    };

    class TabFormatItem : public FormatItem
    {
    public:
        virtual void format(std::ostream &out, const LogMsg &msg) override
        {
            out << "\t";
        }

    private:
        std::string _str;
    };

    /*
        - `%d` 日期
        - `%T` 缩进
        - `%t` 线程 id
        - `%p` 日志级别
        - `%c` 日志器名称
        - `%f` 文件名
        - `%l` 行号
        - `%m` 日志消息
        - `%n` 换行
    */

    class Formatter
    {
    public:
        using ptr = std::shared_ptr<Formatter>;
        Formatter(const std::string &pattern = "[%d{%H:%M:%S}][%t][%c][%f:%l][%p]%T%m%n") : _pattern(pattern)
        {
            assert(parsePattern());
        };
        // 对msg进行格式化

        void format(std::ostream &out, LogMsg &msg)
        {
            for (auto &item : _items)
            {
                item->format(out, msg);
            }
        };
        std::string format(LogMsg &msg)
        {
            std::stringstream ss;
            format(ss, msg);
            return ss.str();
        };

    private:
        // 对格式化规则字符串进行解析
        bool parsePattern()
        {
            _items.clear();
            // 1.对格式化规则字符串进行解析
            // asda%%[%d{%H:%M:%S}][%p]%T%m%n
            std::vector<std::pair<std::string, std::string>> fmt_ord;
            size_t pos = 0;
            const size_t pattern_len = _pattern.size();
            std::string literal;
            auto flush_literal = [&]
            {
                if (!literal.empty())
                {
                    fmt_ord.push_back({"OTHER", literal});
                    literal.clear();
                }
            };

            while (pos < pattern_len)
            {
                std::string key, value;
                char ch = _pattern[pos];
                // 其他输入
                if (_pattern[pos] != '%')
                {
                    literal.push_back(ch);
                    pos++;
                    continue;
                }

                pos++; // 指向%后第一个格式化字符
                if (pos == pattern_len)
                {
                    return false;
                }
                // 如果出现%%情况
                if (_pattern[pos] == '%')
                {
                    // 离开当前指向的%后continue
                    literal.push_back('%');
                    pos++;
                    continue;
                }

                // 确定格式化字符
                key = _pattern[pos];
                // 判断是否进入子格式化字符串
                // 进入子字符串
                flush_literal();
                if (pos + 1 < pattern_len && _pattern[pos + 1] == '{')
                {
                    size_t first_inf_pos = pos + 2;
                    size_t last_inf_pos = _pattern.find_first_of('}', first_inf_pos);
                    if (last_inf_pos == std::string::npos)
                    {
                        return false;
                    }
                    value.assign(_pattern, first_inf_pos, last_inf_pos - first_inf_pos);
                    fmt_ord.push_back({std::move(key), std::move(value)});
                    pos = last_inf_pos + 1;
                    continue;
                }
                // 不进入子字符串
                fmt_ord.push_back({std::move(key), std::move(value)});
                pos++;
            }
            flush_literal();
            // 2.添加到_items中
            for (auto &kv : fmt_ord)
            {
                auto item = createItem(kv.first, kv.second);
                if (!item)
                    return false; // 未知键等错误
                _items.push_back(std::move(item));
            }
            return true;
        };

    private:
        // 根据不同的格式化字符创建不同的格式化子项对象
        FormatItem::ptr createItem(const std::string &key, const std::string &val)
        {
            if (key == "d")
                return std::make_shared<TimeFormatItem>(val);
            else if (key == "t")
                return std::make_shared<ThreadFormatItem>();
            else if (key == "c")
                return std::make_shared<LoggerFormatItem>();
            else if (key == "f")
                return std::make_shared<FileFormatItem>();
            else if (key == "l")
                return std::make_shared<LineFormatItem>();
            else if (key == "p")
                return std::make_shared<LevelFormatItem>();
            else if (key == "T")
                return std::make_shared<TabFormatItem>();
            else if (key == "m")
                return std::make_shared<MsgFormatItem>();
            else if (key == "n")
                return std::make_shared<NLineFormatItem>();
            return std::make_shared<OtherFormatItem>(val);
        };

    private:
        std::string _pattern;
        std::vector<FormatItem::ptr> _items;
    };
}
