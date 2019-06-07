// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

//
//  Animation.h
//  gl
//
//  Created by Fabian on 8/17/13.
//
//

#pragma once

#include <chrono>
#include "crocore/crocore.hpp"
#include "crocore/Easing.hpp"

namespace crocore {

class Animation
{
public:

    using ease_fn_t = std::function<float(float)>;
    using interpolate_fn_t = std::function<void(float)>;
    using callback_fn_t = std::function<void(Animation&)>;

    enum LoopType
    {
        LOOP_NONE = 0, LOOP = 1, LOOP_BACK_FORTH = 2
    };

    enum PlaybackType
    {
        PLAYBACK_PAUSED = 0, PLAYBACK_FORWARD = 1, PLAYBACK_BACKWARD = 2
    };

    template<typename T>
    static Animation create(T *value_ptr, const T &from_value, const T &to_value, float duration)
    {
        return Animation(duration, [=](float progress)
        {
            *value_ptr = crocore::mix(from_value, to_value, progress);
        });
    };

    Animation() = default;

    Animation(float duration, interpolate_fn_t interpolate_fn);

    Animation(Animation &&other) noexcept;

    Animation(const Animation &) = delete;

    Animation &operator=(Animation other);

    friend void swap(Animation &lhs, Animation &rhs);

    /*!
     * Start the animation with an optional delay in seconds
     */
    void start(float delay = 0.f);

    void stop();

    void update();

    float duration() const;

    void set_duration(float d);

    bool is_playing() const;

    Animation::PlaybackType playbacktype() const { return m_playback_type; }

    void set_playback_type(PlaybackType playback_type) { m_playback_type = playback_type; }

    Animation::LoopType loop_type() const { return m_loop_type; }

    void set_loop_type(LoopType loop_type = LOOP) { m_loop_type = loop_type; }

    void set_interpolation_function(interpolate_fn_t fn) { m_interpolate_fn = std::move(fn); }

    void set_ease_function(ease_fn_t fn) { m_ease_fn = std::move(fn); }

    void set_finish_callback(callback_fn_t cb) { m_finish_fn = std::move(cb); }

    float progress() const;

    bool finished() const;

private:

    PlaybackType m_playback_type = PLAYBACK_PAUSED;
    LoopType m_loop_type = LOOP_NONE;
    std::chrono::steady_clock::time_point m_start_time, m_end_time;
    ease_fn_t m_ease_fn = easing::EaseNone();
    interpolate_fn_t m_interpolate_fn;
    callback_fn_t m_finish_fn;
};

}//namespace
