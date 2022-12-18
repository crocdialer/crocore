//
//  CircularBuffer.hpp
//  kinskiGL
//
//  Created by Fabian on 13/09/16.
//
//

#pragma once

#include "crocore/crocore.hpp"

namespace crocore
{

template<typename T, typename Allocator = std::allocator<T>>
class CircularBuffer
{
public:

    using value_type = T;

    CircularBuffer() = default;

    explicit CircularBuffer(uint32_t cap) :
            m_array_size((int32_t)cap + 1),
            m_first(0),
            m_last(0),
            m_data(new T[m_array_size]())
    {

    }

    // copy constructor
    CircularBuffer(const CircularBuffer &other) :
            m_array_size(other.m_array_size),
            m_first(other.m_first),
            m_last(other.m_last),
            m_data(new T[other.m_array_size])
    {
        memcpy(m_data, other.m_data, m_array_size * sizeof(T));
    }

    // move constructor
    CircularBuffer(CircularBuffer &&other) noexcept : CircularBuffer()
    {
        swap(*this, other);
    }

    ~CircularBuffer()
    {
        delete[](m_data);
    }

    CircularBuffer &operator=(CircularBuffer other)
    {
        swap(*this, other);
        return *this;
    }

    inline void clear()
    {
        m_first = m_last = 0;
    }

    template<class InputIt>
    void assign(InputIt first, InputIt last)
    {
        clear();
        std::copy(first, last, std::back_inserter(*this));
    }

    inline void push_back(const T &the_val)
    {
        m_data[m_last] = the_val;
        m_last = (m_last + 1) % m_array_size;

        // buffer is full, drop oldest value
        if(m_first == m_last){ m_first = (m_first + 1) % m_array_size; }
    }

    inline void pop_front()
    {
        if(!empty()){ m_first = (m_first + 1) % m_array_size; }
    }

    inline T &front() { return m_data[m_first]; }

    inline T &back() { return m_data[(m_last - 1) % m_array_size]; }

    inline const T &front() const { return m_data[m_first]; }

    inline const T &back() const { return m_data[(m_last - 1) % m_array_size]; }

    [[nodiscard]] inline uint32_t capacity() const { return std::max(0, m_array_size - 1); };

    inline void set_capacity(uint32_t the_cap) { *this = CircularBuffer(the_cap); }

    [[nodiscard]] inline size_t size() const
    {
        int ret = m_last - m_first;
        if(ret < 0){ ret += m_array_size; }
        return ret;
    };

    [[nodiscard]] inline bool empty() const { return m_first == m_last; }

    inline T &operator[](uint32_t the_index)
    {
        if(the_index >= size()){ throw std::runtime_error("CircularBuffer: index out of bounds"); }
        return m_data[(m_first + the_index) % m_array_size];
    };

    inline const T &operator[](uint32_t the_index) const
    {
        if(the_index >= size()){ throw std::runtime_error("CircularBuffer: index out of bounds"); }
        return m_data[(m_first + the_index) % m_array_size];
    };

    class iterator
    {
    public:

        using iterator_category = std::random_access_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = T*;
        using reference = T&;

        iterator() :
                m_buf(nullptr),
                m_array(nullptr),
                m_ptr(nullptr),
                m_size(0) {}

        iterator(const iterator &the_other) :
                m_buf(the_other.m_buf),
                m_array(the_other.m_array),
                m_ptr(the_other.m_ptr),
                m_size(the_other.m_size) {}

        iterator(const CircularBuffer<T> *the_buf, uint32_t the_pos) :
                m_buf(the_buf),
                m_array(the_buf->m_data),
                m_ptr(the_buf->m_data + the_pos),
                m_size(the_buf->m_array_size) {}

        inline iterator &operator=(iterator the_other)
        {
            std::swap(m_buf, the_other.m_buf);
            std::swap(m_array, the_other.m_array);
            std::swap(m_ptr, the_other.m_ptr);
            std::swap(m_size, the_other.m_size);
            return *this;
        }

        inline iterator &operator++()
        {
            m_ptr++;
            if(m_ptr >= (m_array + m_size)){ m_ptr -= m_size; }
            return *this;
        }

        inline iterator &operator--()
        {
            m_ptr--;
            if(m_ptr < m_array){ m_ptr += m_size; }
            return *this;
        }

        inline bool operator==(const iterator &the_other) const
        {
            return m_array == the_other.m_array && m_ptr == the_other.m_ptr;
        }

        inline bool operator!=(const iterator &the_other) const { return !(*this == the_other); }

        inline T &operator*() { return *m_ptr; }

        inline const T &operator*() const { return *m_ptr; }

        inline T *operator->() { return m_ptr; }

        inline const T *operator->() const { return m_ptr; }

        inline iterator &operator+(int i)
        {
            m_ptr += i;
            if(m_ptr >= (m_array + m_size)){ m_ptr -= m_size; }
            if(m_ptr < m_array){ m_ptr += m_size; }
            return *this;
        }

        inline iterator &operator-(int i)
        {
            return *this + (-i);
        }

        inline int operator-(const iterator &the_other) const
        {
            auto index = m_ptr - m_array - m_buf->m_first;
            index += index < 0 ? m_size : 0;
            auto other_index = the_other.m_ptr - m_array - m_buf->m_first;
            other_index += index < 0 ? m_size : 0;
            return index - other_index;
        }
    private:
        friend CircularBuffer<T>;

        const CircularBuffer<T> *m_buf;
        T *m_array, *m_ptr;
        uint32_t m_size;
    };

    iterator begin() const { return iterator(this, m_first); }

    iterator end() const { return iterator(this, m_last); }

    friend void swap(CircularBuffer<T> &lhs, CircularBuffer<T> &rhs)
    {
        std::swap(lhs.m_array_size, rhs.m_array_size);
        std::swap(lhs.m_first, rhs.m_first);
        std::swap(lhs.m_last, rhs.m_last);
        std::swap(lhs.m_data, rhs.m_data);
    }

private:
    int32_t m_array_size = 0, m_first = 0, m_last = 0;
    T *m_data = nullptr;
};

}// namespace
