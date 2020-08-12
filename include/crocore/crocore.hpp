// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#pragma once

#ifdef WIN32
#define CROC_API __declspec(dllexport)
#else
#define CROC_API
#endif

#include <cstring>
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <algorithm>

//! forward declare a class and define shared-, const-, weak- and unique-pointers for it.
#define DEFINE_CLASS_PTR(CLASS_NAME)\
class CLASS_NAME;\
using CLASS_NAME##Ptr = std::shared_ptr<CLASS_NAME>;\
using CLASS_NAME##ConstPtr = std::shared_ptr<const CLASS_NAME>;\
using CLASS_NAME##WeakPtr = std::weak_ptr<CLASS_NAME>;\
using CLASS_NAME##UPtr = std::unique_ptr<CLASS_NAME>;

// forward declare boost io_service
namespace boost::asio{ class io_context; }
namespace crocore{ using io_service_t = boost::asio::io_context; }

#include "Logger.hpp"
#include "utils.hpp"
