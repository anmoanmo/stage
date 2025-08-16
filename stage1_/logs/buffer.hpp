/*实现异步缓冲区*/

#pragma once
#include <vector>
#include <cstddef>
#include <cstring>
#include <algorithm>
#include <cassert>

namespace mylog
{
    inline constexpr size_t DEFAULT_BUFFER_SIZE = 10 * 1024 * 1024;
    inline constexpr size_t THRESHOLD_BUFFER_SIZE = 80 * 1024 * 1024;
    inline constexpr size_t INCREMENT_BUFFER_SIZE = 10 * 1024 * 1024;
    // inline constexpr size_t MAX_BUFFER_SIZE = 200 * 1024 * 1024;

    class Buffer
    {
    public:
        // 修改：1.0只声明了 Buffer();，没有定义，成员未初始化会 UB
        Buffer(size_t max_size = 200 * 1024 * 1024)
            : _MAX_BUFFER_SIZE(max_size),
              _buffer(std::min(DEFAULT_BUFFER_SIZE, _MAX_BUFFER_SIZE)), // ← 改：初始容量不超过 MAX
              _reader_idx(0),
              _writer_idx(0)
        {
        }

        void resize(size_t max_size)
        {
            _MAX_BUFFER_SIZE = std::max<size_t>(1, max_size); // 给个下限
            // 不主动缩容 _buffer；让后续扩容受新上限约束即可
        }

        // 向缓冲区写入数据
        bool push(const char *data, size_t len)
        {
            // 缓冲区剩余空间不够：1.扩容；2.阻塞/返回false;
            // 1.固定大小，则直接返回
            // 2.动态空间，用于极限测试--扩容
            ensureEnoughSize(len);
            if (writerableSize() < len)
            {
                return false;
            }

            // 1.将当前数据拷贝仅缓冲区
            std::memcpy(_buffer.data() + _writer_idx, data, len);
            // 2.将当前写入指针进行向后偏移
            moveWriter(len);
            return true;
        };
        size_t writerableSize() const
        {
            // 仅针对固定大小缓冲区提供，因为可扩容缓冲区总是可写
            return (_buffer.size() - _writer_idx);
        };
        // 返回可读数据的起始地址
        const char *readPtr() const
        {
            return _buffer.data() + _reader_idx;
            ;
        };

        size_t readableSize() const
        {
            // 缓存区线性向后，处理完后交换
            return (_writer_idx - _reader_idx);
        };
        // 对读写指针进行向后偏移操作
        void moveWriter(size_t len)
        {
            assert(len <= writerableSize());
            _writer_idx += len;
        };
        void moveReader(size_t len)
        {
            assert(len <= readableSize());
            _reader_idx += len;
            // 可选：读尽即复位
            if (_reader_idx == _writer_idx)
            {
                reset();
            }
            assert(_reader_idx <= _writer_idx);
            assert(_writer_idx <= _buffer.size());
        };
        // 重置读写位置，初始化缓冲区
        void reset()
        {
            _reader_idx = 0; // 与_writer_idx相同，没有内容可读
            _writer_idx = 0; // 所有空间都是空闲的
        };
        void swap(Buffer &other)
        {
            _buffer.swap(other._buffer);
            std::swap(_reader_idx, other._reader_idx);
            std::swap(_writer_idx, other._writer_idx);
            std::swap(_MAX_BUFFER_SIZE, other._MAX_BUFFER_SIZE);
        };
        // 判断缓冲区是否为空
        bool empty() const
        {
            return (_reader_idx == _writer_idx);
        };

    private:
        // 对空间进行扩容操作
        void ensureEnoughSize(size_t need)
        {
            if (need > _MAX_BUFFER_SIZE)
            {
                return; // 这次写入永远放不下，交给调用方处理（push 返回 false）
            }

            // 有效阈值/增量（防冲突）
            const size_t threshold_eff = std::max<size_t>(1, std::min(THRESHOLD_BUFFER_SIZE, _MAX_BUFFER_SIZE));
            const size_t increment_eff = std::max<size_t>(1, std::min(INCREMENT_BUFFER_SIZE, _MAX_BUFFER_SIZE));

            while (writerableSize() < need && _buffer.size() < _MAX_BUFFER_SIZE)
            {
                size_t cur = _buffer.size();
                size_t new_size;
                if (cur < threshold_eff)
                {
                    // 指数增长但不超过阈值
                    new_size = std::min(cur * 2, threshold_eff);
                }
                else
                {
                    // 线性增长
                    new_size = cur + increment_eff;
                }
                // 满足本次需求
                new_size = std::max(new_size, _writer_idx + need);
                // 不超过上限
                new_size = std::min(new_size, _MAX_BUFFER_SIZE);

                if (new_size <= cur)
                {
                    break; // 防死循环（极端情况下）
                }
                _buffer.resize(new_size);
            }
        }

        size_t _MAX_BUFFER_SIZE;
        std::vector<char> _buffer;
        size_t _reader_idx; // 本质是下标
        size_t _writer_idx;
    };
}