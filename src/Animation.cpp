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
using float_second = std::chrono::duration<float>;

namespace crocore {

Animation::Animation(float duration, float delay, interpolate_fn_t interpolate_fn) :
        m_playback_type(PLAYBACK_PAUSED),
        m_loop_type(LOOP_NONE),
        m_start_time(steady_clock::now() + duration_cast<steady_clock::duration>(float_second(delay))),
        m_end_time(m_start_time + duration_cast<steady_clock::duration>(float_second(duration))),
        m_ease_fn(animation::EaseNone()),
        m_interpolate_fn(std::move(interpolate_fn))
{

}

float Animation::duration() const
{
    return duration_cast<float_second>(m_end_time - m_start_time).count();
}

void Animation::set_duration(float d)
{
    m_end_time = m_start_time + duration_cast<steady_clock::duration>(float_second(d));
}

bool Animation::is_playing() const
{
    return m_playback_type && (steady_clock::now() >= m_start_time);
}

float Animation::progress() const
{
    return clamp(duration_cast<float_second>(steady_clock::now() - m_start_time).count() /
                 duration_cast<float_second>(m_end_time - m_start_time).count(),
                 0.f, 1.f);
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
        if(m_playback_type == PLAYBACK_FORWARD && m_finish_fn){ m_finish_fn(); }
        else if(m_playback_type == PLAYBACK_BACKWARD && m_reverse_finish_fn){ m_reverse_finish_fn(); }

        if(loop())
        {
            if(m_loop_type == LOOP_BACK_FORTH)
            {
                m_playback_type = m_playback_type == PLAYBACK_FORWARD ?
                                  PLAYBACK_BACKWARD : PLAYBACK_FORWARD;
            }
            start();
        }else
        {
            // end playback
            stop();
        }
    }

    // get current progress, eventually revert it
    float val = progress();
    if(m_playback_type == PLAYBACK_BACKWARD){ val = 1.f - val; }

    // optionally apply easing
    if(m_ease_fn){ val = m_ease_fn(val); }

    // pass the value to an interpolation function
    if(m_interpolate_fn){ m_interpolate_fn(val); }

    // fire update callback, if any
    if(m_update_fn){ m_update_fn(); }
}

/*!
 * Start the animation with an optional delay in seconds
 */
void Animation::start(float delay)
{
    if(!is_playing()){ m_playback_type = PLAYBACK_FORWARD; }

    float dur = duration();
    m_start_time = steady_clock::now() + duration_cast<steady_clock::duration>(float_second(delay));
    m_end_time = m_start_time + duration_cast<steady_clock::duration>(float_second(dur));

    // fire start callback, if any
    if(m_playback_type == PLAYBACK_FORWARD && m_start_fn){ m_start_fn(); }
    else if(m_playback_type == PLAYBACK_BACKWARD && m_reverse_start_fn){ m_reverse_start_fn(); }

}

void Animation::stop()
{
    m_playback_type = PLAYBACK_PAUSED;
}

}//namespaces
