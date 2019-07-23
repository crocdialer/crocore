// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

//  Animation.h
//
//  Created by Fabian on 8/17/13.

#include "crocore/Animation.hpp"

using std::chrono::duration_cast;
using std::chrono::microseconds;
using std::chrono::steady_clock;

// ratio is 1 second per second, wow :D
using double_second = std::chrono::duration<double>;

namespace crocore {

Animation::Animation(double duration, interpolate_fn_t interpolate_fn) :
        m_playback_type(PLAYBACK_PAUSED),
        m_loop_type(LOOP_NONE),
        m_start_time(steady_clock::now()),
        m_end_time(m_start_time + duration_cast<steady_clock::duration>(double_second(duration))),
        m_ease_fn(easing::EaseNone()),
        m_interpolate_fn(std::move(interpolate_fn))
{

}

Animation::Animation(Animation &&other) noexcept:
        Animation()
{
    swap(*this, other);
}

Animation &Animation::operator=(Animation other)
{
    swap(*this, other);
    return *this;
}

void swap(Animation &lhs, Animation &rhs)
{
    std::swap(lhs.m_playback_type, rhs.m_playback_type);
    std::swap(lhs.m_loop_type, rhs.m_loop_type);
    std::swap(lhs.m_start_time, rhs.m_start_time);
    std::swap(lhs.m_end_time, rhs.m_end_time);
    std::swap(lhs.m_ease_fn, rhs.m_ease_fn);
    std::swap(lhs.m_interpolate_fn, rhs.m_interpolate_fn);
    std::swap(lhs.m_finish_fn, rhs.m_finish_fn);
}

double Animation::duration() const
{
    return duration_cast<double_second>(m_end_time - m_start_time).count();
}

void Animation::set_duration(double d)
{
    auto new_duration = double_second(d);
    auto frac = progress();
    auto frac_duration = new_duration * frac;
    m_start_time = steady_clock::now() - duration_cast<steady_clock::duration>(frac_duration);
    m_end_time = m_start_time + duration_cast<steady_clock::duration>(new_duration);
}

bool Animation::is_playing() const
{
    return m_playback_type && (steady_clock::now() >= m_start_time);
}

double Animation::progress(bool eased) const
{
    auto val = clamp(duration_cast<double_second>(steady_clock::now() - m_start_time).count() /
                     duration_cast<double_second>(m_end_time - m_start_time).count(),
                     0., 1.);

    // optionally apply easing
    if(eased && m_ease_fn){ val = m_ease_fn(val); }

    // eventually revert it
    if(m_playback_type == PLAYBACK_BACKWARD){ val = 1. - val; }

    return val;
}

bool Animation::finished() const
{
    return steady_clock::now() >= m_end_time;
}

void Animation::update()
{
    if(!is_playing()) return;

    if(finished())
    {
        // fire finish callback, if any
        if(m_finish_fn){ m_finish_fn(*this); }

        if(loop_type())
        {
            if(m_loop_type == LOOP_BACK_FORTH)
            {
                m_playback_type = m_playback_type == PLAYBACK_FORWARD ?
                                  PLAYBACK_BACKWARD : PLAYBACK_FORWARD;
            }

            auto duration = m_end_time - m_start_time;
            auto frac = (steady_clock::now() - m_end_time) % duration;
            m_start_time = steady_clock::now() - frac;
            m_end_time = m_start_time + duration;
        }else
        {
            // end playback
            stop();
        }
    }

    // get current progress with easing and playtype applied
    double val = progress(true);

    // pass the value to an interpolation function
    if(m_interpolate_fn){ m_interpolate_fn(val); }
}

/*!
 * Start the animation with an optional delay in seconds
 */
void Animation::start(double delay)
{
    if(!is_playing()){ m_playback_type = PLAYBACK_FORWARD; }

    auto duration = m_end_time - m_start_time;
    m_start_time = steady_clock::now() + duration_cast<steady_clock::duration>(double_second(delay));
    m_end_time = m_start_time + duration;
}

void Animation::stop()
{
    m_playback_type = PLAYBACK_PAUSED;
}

}//namespaces
