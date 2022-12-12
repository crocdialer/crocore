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
#include <functional>
#include <algorithm>

// forward declare boost io_service
namespace boost::asio{ class io_context; }
namespace crocore{ using io_service_t = boost::asio::io_context; }

#include "utils.hpp"
#include "define_class_ptr.hpp"
