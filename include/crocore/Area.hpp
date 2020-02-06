//
//  Header.h
//  core
//
//  Created by Fabian on 22.03.18.
//

#pragma once

#include "crocore/crocore.hpp"

namespace crocore
{

template<typename T>
class Area_
{
public:
    T x = 0, y = 0, width = 0, height = 0;

    explicit operator bool() const{ return width != T(0) && height != T(0); }

    inline bool operator==(const Area_<T> &other) const
    {
        return !(*this != other);
    }

    inline bool operator!=(const Area_<T> &other) const
    {
        return x != other.x || y != other.y || width != other.width || height != other.height;
    }
};

}// namespace crocdialer
