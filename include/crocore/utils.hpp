// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

//  Utils.h
//
//  Created by Fabian on 11/11/13.

#pragma once

#include "crocore.hpp"
#include <iomanip>
#include <memory>
#include <numeric>
#include <random>
#include <sstream>

namespace crocore
{

#ifdef NDEBUG
#define CROCORE_IF_DEBUG(...)
#else
#define CROCORE_IF_DEBUG(...)		__VA_ARGS__
#endif

/**
 * @brief   wait for completion of all tasks, represented by their futures
 * @param   tasks   the provided futures to wait for
 */
template<typename Collection>
inline void wait_all(const Collection &futures)
{
    for(const auto &f: futures)
    {
        if(f.valid()){ f.wait(); }
    }
}

template<typename T>
std::vector<uint8_t> to_bytes(const T &t)
{
    std::vector<uint8_t> ret(sizeof(t));
    memcpy(ret.data(), &t, sizeof(t));
    return ret;
}

template<typename T>
inline std::string to_string(const T &theObj, int precision = 0)
{
    std::stringstream ss;

    if(precision > 0)
    {
        if(std::is_integral<T>()) { ss << std::setfill('0') << std::setw(precision); }
        else if(std::is_floating_point<T>()) { ss << std::fixed << std::setprecision(precision); }
    }
    ss << theObj;
    return ss.str();
}

template<typename T>
inline T string_to(const std::string &str)
{
    T ret;
    std::stringstream ss(str);
    ss >> ret;
    return ret;
}

template<typename... Args>
inline std::string format(const std::string &the_format, Args... args)
{
    int size = snprintf(nullptr, 0, the_format.c_str(), args...) + 1;
    std::unique_ptr<char[]> buf(new char[size]);
    snprintf(buf.get(), size, the_format.c_str(), args...);
    return std::string(buf.get(), buf.get() + size - 1);
}

inline std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems,
                                       bool remove_empty_splits = true)
{
    std::stringstream ss(s);
    std::string item;
    while(std::getline(ss, item, delim))
    {
        if(!item.empty() || !remove_empty_splits) elems.push_back(item);
    }
    return elems;
}

inline std::vector<std::string> split(const std::string &s, char delim = ' ', bool remove_empty_splits = true)
{
    std::vector<std::string> elems;
    split(s, delim, elems, remove_empty_splits);
    return elems;
}

inline std::vector<std::string> split_by_string(const std::string &s, const std::string &delim = " ")
{
    std::vector<std::string> elems;
    std::string::size_type pos = s.find(delim), current_pos = 0;

    while(pos != std::string::npos)
    {
        elems.push_back(s.substr(current_pos, pos - current_pos));
        current_pos = pos + delim.size();

        // continue searching
        pos = s.find(delim, current_pos);
    }

    // remainder
    std::string remainder = s.substr(current_pos, s.size() - current_pos);
    if(!remainder.empty()) { elems.push_back(remainder); }
    return elems;
}

inline std::string remove_whitespace(const std::string &input)
{
    std::string ret(input);
    ret.erase(std::remove_if(ret.begin(), ret.end(), [](char c) { return std::isspace(c); }), ret.end());
    return ret;
}

inline std::string trim(const std::string &str, const std::string &whitespace = " \t")
{
    auto str_begin = str.find_first_not_of(whitespace);
    if(str_begin == std::string::npos) { str_begin = 0; }
    auto str_end = str.find_last_not_of(whitespace);
    if(str_end == std::string::npos) { str_end = str.size() - 1; }
    const auto str_range = str_end - str_begin + 1;
    return str.substr(str_begin, str_range);
}

inline std::string to_lower(const std::string &str)
{
    std::string ret = str;
    std::transform(str.begin(), str.end(), ret.begin(),
                   [](char c) -> char { return static_cast<char>(std::tolower(c)); });
    return ret;
}

inline std::string to_upper(const std::string &str)
{
    std::string ret = str;
    std::transform(str.begin(), str.end(), ret.begin(),
                   [](char c) -> char { return static_cast<char>(std::toupper(c)); });
    return ret;
}


inline std::string secs_to_time_str(float the_secs)
{
    return format("%d:%02d:%04.1f", (int) the_secs / 3600, ((int) the_secs / 60) % 60, fmodf(the_secs, 60));
}

inline constexpr bool is_pow_2(size_t v) { return !(v & (v - 1)); }

inline constexpr uint64_t next_pow_2(uint64_t v)
{
    if(is_pow_2(v)) { return v; }
    v--;
    v |= v >> 1U;
    v |= v >> 2U;
    v |= v >> 4U;
    v |= v >> 8U;
    v |= v >> 16U;
    v |= v >> 32U;
    v++;
    return v;
}

template<typename T>
inline T swap_endian(T u)
{
    union
    {
        T u;
        unsigned char u8[sizeof(T)];
    } source, dest;
    source.u = u;

    for(size_t k = 0; k < sizeof(T); k++) { dest.u8[k] = source.u8[sizeof(T) - k - 1]; }
    return dest.u;
}

inline static void swap_endian(void *dest, const void *src, size_t num_bytes)
{
    auto tmp = new uint8_t[num_bytes];
    auto src_ptr = (const uint8_t *) src;

    for(size_t i = 0; i < num_bytes; ++i) { tmp[i] = src_ptr[num_bytes - 1 - i]; }
    memcpy(dest, tmp, num_bytes);
    delete[](tmp);
}

inline static uint8_t crc8(const uint8_t *buff, size_t size)
{
    uint8_t *p = (uint8_t *) buff;
    uint8_t result = 0xFF;

    for(result = 0; size != 0; size--)
    {
        result ^= *p++;

        for(size_t i = 0; i < 8; i++)
        {
            if(result & 0x80)
            {
                result <<= 1;
                result ^= 0x85;// x8 + x7 + x2 + x0
            }
            else { result <<= 1; }
        }
    }
    return result;
}

inline static uint16_t crc16(const uint8_t *buff, size_t size)
{
    uint8_t *data = (uint8_t *) buff;
    uint16_t result = 0xFFFF;

    for(size_t i = 0; i < size; ++i)
    {
        result ^= data[i];

        for(size_t j = 0; j < 8; ++j)
        {
            if(result & 0x01) { result = (result >> 1) ^ 0xA001; }
            else { result >>= 1; }
        }
    }
    return result;
}

template<typename T, typename C>
inline bool contains(const C &container, const T &elem)
{
    return std::find(std::begin(container), std::end(container), elem) != std::end(container);
}

namespace details
{
// allow in-order expansion of parameter packs.
struct do_in_order
{
    template<typename T>
    do_in_order(std::initializer_list<T> &&)
    {}
};

template<typename C1, typename C2>
void concat_helper(C1 &l, const C2 &r)
{
    l.insert(std::end(l), std::begin(r), std::end(r));
}
}// namespace details

template<typename T, typename... C>
inline std::vector<T> concat_containers(C &&...containers)
{
    std::vector<T> ret;
    details::do_in_order{(details::concat_helper(ret, std::forward<C>(containers)), 0)...};
    return ret;
}

template<typename C>
inline auto sum(const C &container)
{
    using elem_t = typename C::value_type;
    return std::accumulate(std::begin(container), std::end(container), elem_t(0));
}

template<typename C>
inline auto mean(const C &container)
{
    using elem_t = typename C::value_type;
    auto size = static_cast<double>(std::end(container) - std::begin(container));
    return size > 0 ? sum(container) / elem_t(size) : elem_t(0);
}

template<typename T = double, typename C>
inline T standard_deviation(const C &the_container)
{
    auto mean = crocore::mean(the_container);
    std::vector<T> diff(std::begin(the_container), std::end(the_container));
    std::transform(std::begin(the_container), std::end(the_container), diff.begin(), [mean](T x) { return x - mean; });
    T sq_sum = std::inner_product(diff.begin(), diff.end(), diff.begin(), T(0));
    T stdev = std::sqrt(sq_sum / (T) diff.size());
    return stdev;
}

template<typename T = double, typename C>
inline T median(const C &the_container)
{
    std::vector<T> tmp_array(std::begin(the_container), std::end(the_container));
    if(tmp_array.empty()) { return T(0); }
    size_t n = tmp_array.size() / 2;
    std::nth_element(tmp_array.begin(), tmp_array.begin() + n, tmp_array.end());

    if(tmp_array.size() % 2) { return tmp_array[n]; }
    else
    {
        // even sized vector -> average the two middle values
        auto max_it = std::max_element(tmp_array.begin(), tmp_array.begin() + n);
        return (*max_it + tmp_array[n]) / T(2);
    }
}

template<typename T = float>
inline T halton(uint32_t index, uint32_t base)
{
    T f = 1;
    T r = 0;
    uint32_t current = index;

    while(current)
    {
        f = f / static_cast<float>(base);
        r = r + f * static_cast<float>(current % base);
        current /= base;
    }
    return r;
}

template<typename T>
inline int sgn(T val)
{
    return (T(0) < val) - (val < T(0));
}

template<typename T>
inline constexpr const T &clamp(const T &val, const T &min, const T &max)
{
    return val < min ? min : (val > max ? max : val);
}

template<typename T, typename Real = float>
inline T mix(const T &lhs, const T &rhs, Real ratio)
{
    return lhs + ratio * (rhs - lhs);
}

template<typename T, typename Real = float>
inline T mix_slow(const T &lhs, const T &rhs, Real ratio)
{
    return lhs * (Real(1) - ratio) + rhs * ratio;
}

template<typename T>
inline T map_value(const T &val, const T &src_min, const T &src_max, const T &dst_min, const T &dst_max)
{
    float mix_val = clamp<float>((val - src_min) / (float) (src_max - src_min), 0.f, 1.f);
    return mix_slow<T>(dst_min, dst_max, mix_val);
}

template<typename T = double>
inline T random(const T &min, const T &max)
{
    // Seed with a real random value, if available
    std::random_device r;

    // random mean
    std::default_random_engine e1(r());
    std::uniform_real_distribution<T> uniform_dist(T(0), std::nextafter(T(1), std::numeric_limits<T>::max()));
    return mix<T, T>(min, max, uniform_dist(e1));
}

template<typename T = int32_t>
inline T random_int(const T &min, const T &max)
{
    // Seed with a real random value, if available
    std::random_device r;

    // random mean
    std::default_random_engine e1(r());
    std::uniform_int_distribution<T> uniform_dist(min, max);
    return uniform_dist(e1);
}

#if defined(unix) || defined(__unix__) || defined(__unix)
inline std::string syscall(const std::string &cmd)
{
    std::string ret;
    std::shared_ptr<FILE> pipe(popen(cmd.c_str(), "r"), pclose);
    char buf[1024] = "\0";
    while(fgets(buf, sizeof(buf), pipe.get())) { ret.append(buf); }
    return ret;
}
#endif

// wrapper functions for aligned memory allocation
static inline void *aligned_alloc(size_t size, size_t alignment)
{
    void *data = nullptr;
#if defined(_MSC_VER) || defined(__MINGW32__)
    data = _aligned_malloc(size, alignment);
#else
    int res = posix_memalign(&data, alignment, size);
    if(res != 0) { data = nullptr; }
#endif
    return data;
}

static inline void aligned_free(void *data)
{
#if defined(_MSC_VER) || defined(__MINGW32__)
    _aligned_free(data);
#else
    free(data);
#endif
}

}// namespace crocore
