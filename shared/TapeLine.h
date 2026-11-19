/*
  ==============================================================================

    TapeLine.h
    Created: 20 Jun 2026 12:34:00am
    Author:  julian

    Contains (modified) code originally by Emilie Gillet (Copyright 2014, emilie.o.gillet@gmail.com) under MIT License:

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.

  ==============================================================================
*/

#pragma once

#include <cstdlib>
#include <random>
#include <algorithm>

#include "stmlib/stmlib.h"
#include "stmlib/dsp/dsp.h"

#include "SingleSampleFilter.h"
#include "HelperFunctions.h"

#define TIME_STEP_SIZE 0.00001f
#define MAX_DRIVE_GAIN 8.0f
#define DRIVE_COMPENSATION 7.0f
#define HP_BASE_FREQ 0.0f
#define HP_MAX_FREQ 1000.0f
#define LP_BASE_FREQ 20000.0f
#define LP_MAX_FREQ 900.0f
#define BASE_RESONANCE 0.9f
#define MAX_RESONANCE 1.8f
#define FLUCTUATE_ODDS 3
#define MIN_LOOP_FACTOR 0.01f
#define MAX_LOOP_FACTOR 4.00f
#define MIN_FADE_RATIO 0.009f
#define MAX_FADE_RATIO 0.4909f

using namespace helper;

template<typename T, size_t max_delay>
class MonoTapeLine {
public:
    MonoTapeLine() {}
    ~MonoTapeLine() {}

    void reset(T* buffer) {
        this->line = buffer;
        std::fill(&line[0], &line[max_delay], T(0));
        write_ptr = 0;
    }

    void write(const T& in) {
        line[write_ptr] = in;
        write_ptr = (write_ptr - 1 + max_delay) % max_delay;
    }

    void readInterpolated(T& out, float delay_ratio) {
        float delay_time = setGetAbsoluteDelayFromRatio(delay_ratio);

        MAKE_INTEGRAL_FRACTIONAL(delay_time)
            int32_t t = (write_ptr + delay_time_integral + max_delay);
        const T xm1 = line[(t - 1) % max_delay];
        const T x0 = line[(t) % max_delay];
        const T x1 = line[(t + 1) % max_delay];
        const T x2 = line[(t + 2) % max_delay];
        const float c = (x1 - xm1) * 0.5f;
        const float v = x0 - x1;
        const float w = c + v;
        const float a = w + v + (x2 - x0) * 0.5f;
        const float b_neg = w + a;
        const float f = delay_time_fractional;
        out = (((a * f) - b_neg) * f + c) * f + x0;
    }

    void readSample(T& out, int delay_time_int) {
        out = line[(write_ptr + max_delay + delay_time_int) % max_delay];
    }

    void readSampleFloat(T& out, float delay_time_float) {
        float delay_time = delay_time_float;

        MAKE_INTEGRAL_FRACTIONAL(delay_time)
            int32_t t = (write_ptr + delay_time_integral + max_delay);
        const T xm1 = line[(t - 1) % max_delay];
        const T x0 = line[(t) % max_delay];
        const T x1 = line[(t + 1) % max_delay];
        const T x2 = line[(t + 2) % max_delay];
        const float c = (x1 - xm1) * 0.5f;
        const float v = x0 - x1;
        const float w = c + v;
        const float a = w + v + (x2 - x0) * 0.5f;
        const float b_neg = w + a;
        const float f = delay_time_fractional;
        out = (((a * f) - b_neg) * f + c) * f + x0;
    }

    float setGetAbsoluteDelayFromRatio(float delay_ratio) {
        current_effective_delay_ratio = delay_ratio;
        return current_absolute_delay = (float)max_delay * delay_ratio;
    }

private:
    size_t write_ptr;
    T* line;
    float step_size;
    float current_absolute_delay = 0.5f;
    float current_effective_delay_ratio = 0.5f;

    DISALLOW_COPY_AND_ASSIGN(MonoTapeLine);
};

enum CoreState{
    DELAY,
    LOOPER,
    ADDING,
    REMOVING,
    FADEOUT,
    IDLE
};

template<typename T, size_t max_delay>
class MonoTapeCore : public MonoTapeLine<T, max_delay> {
public:

    MonoTapeCore<T, max_delay>() :
        master_core(*this)
    {
    };

    MonoTapeCore<T, max_delay>(MonoTapeCore<T, max_delay>& master_core) :
        master_core(master_core)
    {

    };

    void reset(double sample_rate, T* buffer) {
        this->MonoTapeLine<T, max_delay>::reset(buffer);
        this->sample_rate = sample_rate;
        hp.Init();
        lp.Init();
    };

    void setLoopPoint(float current_effective_delay_ratio, float fade_ratio, bool sync_to_master_core = false) {
        this->current_effective_delay_ratio = initial_effective_delay_ratio = current_effective_delay_ratio;
        loop_duration = (float)max_delay * current_effective_delay_ratio;
        fade_duration = loop_duration * MAX_FADE_RATIO * fade_ratio;
        loop_1_head = loop_duration;
        loop_2_head = loop_duration - fade_duration;
        rng = 1.0f;
        loop_2_offset = loop_duration - fade_duration;
        is_synced_to_master_core = sync_to_master_core;
    };

    void updateInitialDelayRatio(float initial_effective_delay_ratio) {
        this->initial_effective_delay_ratio = this->initial_effective_delay_ratio * (initial_effective_delay_ratio / current_effective_delay_ratio);
    };

    void setInitialDelayRatio(float initial_effective_delay_ratio) {
        this->initial_effective_delay_ratio = initial_effective_delay_ratio;
    }

    void readLoop(
        T& out,
        float delay_ratio,
        float fade_ratio,
        bool pitch_relative_to_delay_ratio,
        bool fluctuate,
        bool sync_to_master_core
    ) {
        if (pitch_relative_to_delay_ratio) {
            current_effective_delay_ratio = delay_ratio;
        }
        float time_increment = initial_effective_delay_ratio / clamp(current_effective_delay_ratio, MIN_LOOP_FACTOR, MAX_LOOP_FACTOR);
        loop_1_head = loop_1_head - time_increment;

        // always reset loop 1 head to position of previous loop 2, update fade duration, sync status, rng values & position of next fade-in loop
        if (loop_1_head <= 0.0f) {
            fade_duration = loop_duration * (MIN_FADE_RATIO + MAX_FADE_RATIO * fade_ratio);
            if(is_synced_to_master_core) {
                loop_1_head = loop_2_head - time_increment;
                loop_2_head = master_core.loop_2_head;
                loop_2_offset = master_core.loop_2_offset;
                rng = master_core.rng;
            }
            else {
                loop_1_head = loop_2_head - time_increment;
                if (fluctuate) {
                    if (std::rand() % FLUCTUATE_ODDS == 0) {
                        rng = ((float)std::rand() / (float)RAND_MAX);
                        loop_2_head = loop_2_offset = loop_duration - fade_duration - rng * (loop_duration - 2.0f * fade_duration);
                    }
                }
                else {
                    loop_2_head = loop_2_offset = loop_duration - fade_duration;
                }
            }
        }

        T loop_1;
        this->readSampleFloat(loop_1, std::fmod(std::fmod(loop_1_head, loop_duration), (float)max_delay));

        // fade out of loop 1 and fade into loop 2
        if (loop_1_head < fade_duration) {
            is_synced_to_master_core = sync_to_master_core;

            float fade_progress = loop_1_head / fade_duration;
            T loop_2;
            if (is_synced_to_master_core) {
                loop_2_offset = master_core.loop_2_offset;
                rng = master_core.rng;

                if (master_core.loop_1_head < master_core.fade_duration) {
                    // master core its ahead in its fade cycle, synced core needs to fade into the same blend the master is in to avoid loop reset clicks
                    T master_loop_1, master_loop_2;
                    loop_2_head = master_core.loop_2_head;
                    float master_fade_progress = master_core.loop_1_head / master_core.fade_duration;
                    this->readSampleFloat(master_loop_1, std::fmod(std::fmod(master_core.loop_1_head, loop_duration), (float)max_delay));
                    this->readSampleFloat(master_loop_2, std::fmod(std::fmod(master_core.loop_2_head, loop_duration), (float)max_delay));
                    loop_2 = cosineBlend<float>(master_loop_1, master_loop_2, master_fade_progress);
                }
                else {
                    // save to fade in master core's most recent loop position since loop resets can't be reached
                    loop_2_head = master_core.loop_1_head;
                    this->readSampleFloat(loop_2, std::fmod(std::fmod(loop_2_head, loop_duration), (float)max_delay));
                }
            }
            else {
                // master core always fades into loop 2 directly
                loop_2_head = loop_1_head + loop_2_offset;
                this->readSampleFloat(loop_2, std::fmod(std::fmod(loop_2_head, loop_duration), (float)max_delay));
            }
            out = cosineBlend<float>(loop_1, loop_2, fade_progress);
        }
        else {
            // upade fade duration only if the fade isn't active
            if (is_synced_to_master_core) {
                fade_duration = master_core.fade_duration;
                loop_2_offset = master_core.loop_2_offset;
                rng = master_core.rng;
                if (master_core.loop_1_head < master_core.fade_duration) {
                    // synced core continues master core fade even out of main fade phase
                    float master_fade_progress = master_core.loop_1_head / master_core.fade_duration;
                    T master_loop_1;
                    this->readSampleFloat(master_loop_1, std::fmod(std::fmod(master_core.loop_1_head, loop_duration), (float)max_delay));
                    out = cosineBlend(master_loop_1, loop_1, master_fade_progress);
                }
                else {
                    out = loop_1;
                }
            }
            else {
                fade_duration = loop_duration * (MIN_FADE_RATIO + MAX_FADE_RATIO * fade_ratio);
                if (fluctuate) {
                    // update fade length although rng only changes on loop 1 reset
                    loop_2_offset = loop_duration - fade_duration - rng * (loop_duration - 2.0f * fade_duration);
                }
                else {
                    loop_2_offset = loop_duration - fade_duration;
                }
                out = loop_1;
            }
        }
    };

    void writeWithDrive(const T& in, float drive, float hp_cutoff, float lp_cutoff, float resonance, float in_post_filter) {
        float filtered;
        float hp_freq = (HP_BASE_FREQ + HP_MAX_FREQ * hp_cutoff) / sample_rate;
        float lp_freq = (LP_BASE_FREQ - (LP_BASE_FREQ - LP_MAX_FREQ) * lp_cutoff) / sample_rate;
        hp.set_f_q<SingleSampleFilter::FREQUENCY_FAST>(hp_freq, BASE_RESONANCE + resonance * MAX_RESONANCE);
        lp.set_f_q<SingleSampleFilter::FREQUENCY_FAST>(lp_freq, BASE_RESONANCE + resonance * MAX_RESONANCE);

        filtered = lp.Process<SingleSampleFilter::FILTER_MODE_LOW_PASS>(hp.Process<SingleSampleFilter::FILTER_MODE_HIGH_PASS>(in));
        this->MonoTapeLine<T, max_delay>::write(
            tapeResponse(filtered + in_post_filter, drive)
        );
    };

private:
    float current_effective_delay_ratio;
    float initial_effective_delay_ratio;
    float loop_1_head;
    float loop_2_head;
    float loop_duration;
    float fade_duration;
    float loop_2_offset;
    float rng;
    bool is_synced_to_master_core;
    MonoTapeCore<T, max_delay>& master_core;

    SingleSampleFilter::Svf hp;
    SingleSampleFilter::Svf lp;

    double sample_rate = 48000;


    T tapeResponse(const T& in, float drive) {
        return stmlib::SoftClip(stmlib::SoftClip((1.0 + MAX_DRIVE_GAIN * drive) * in)
            / (1.0f + DRIVE_COMPENSATION * drive));
    };
};

template<typename T, size_t max_delay>
class StereoTapeCore{
public:
    CoreState state;

    StereoTapeCore() :
        shared_loop_head(0.0f),
        shared_fluctuation_offset(0.0f),
        core_l(),
        core_r(core_l)
    {

    }

    void reset(double sample_rate, float* buffer_l, float* buffer_r) {
        core_l.reset(sample_rate, buffer_l);
        core_r.reset(sample_rate, buffer_r);
        sample_rate = sample_rate;
    }

    void setLoopPoint(float current_effective_delay_ratio, float fade_ratio, bool stereo_fluctuate) {
        state = LOOPER;
        core_l.setLoopPoint(current_effective_delay_ratio, fade_ratio);
        core_r.setLoopPoint(current_effective_delay_ratio, fade_ratio, stereo_fluctuate);
    }

    void updateInitialDelayRatio(float initial_effective_delay_ratio) {
        core_l.updateInitialDelayRatio(initial_effective_delay_ratio);
        core_r.updateInitialDelayRatio(initial_effective_delay_ratio);
    }

    void setInitialDelayRatio(float initial_effective_delay_ratio) {
        core_l.setInitialDelayRatio(initial_effective_delay_ratio);
        core_r.setInitialDelayRatio(initial_effective_delay_ratio);
    }

    void removeLoop() {
        state = IDLE;
    }

    void readLoop(
        T& out_l, 
        T& out_r, 
        float delay_ratio_l, 
        float delay_ratio_r, 
        float fade_ratio, 
        bool pitch_relative_to_delay_ratio,
        bool fluctuate, 
        bool stereo_fluctuate
    ) {
        core_l.readLoop(out_l, delay_ratio_l, fade_ratio, pitch_relative_to_delay_ratio, fluctuate, false);
        core_r.readLoop(out_r, delay_ratio_r, fade_ratio, pitch_relative_to_delay_ratio, fluctuate, !stereo_fluctuate);
    }

    void writeWithDrive(const T& in_l, const T& in_r, float drive, float hp_cutoff, float lp_cutoff, float resonance, float in_post_filter_l = 0.0f, float in_post_filter_r = 0.0f) {
        core_l.writeWithDrive(in_l, drive, hp_cutoff, lp_cutoff, resonance, in_post_filter_l);
        core_r.writeWithDrive(in_r, drive, hp_cutoff, lp_cutoff, resonance, in_post_filter_r);
    }

    void write(const T& in_l, const T& in_r) {
        core_l.write(in_l);
        core_r.write(in_r);
    }

    void readInterpolated(T& out_l, T& out_r, float delay_ratio_l, float delay_ratio_r) {
        core_l.readInterpolated(out_l, delay_ratio_l);
        core_r.readInterpolated(out_r, delay_ratio_r);
    }

    float getMeanDelaySamples() {
        return (core_l.getAbsoluteDelay() + core_r.getAbsoluteDelay()) / 2.0f;
    }

private:
    double sample_rate = 48000;
    MonoTapeCore<T, max_delay> core_l;
    MonoTapeCore<T, max_delay> core_r;
    float shared_loop_head;
    float shared_fluctuation_offset;
};

