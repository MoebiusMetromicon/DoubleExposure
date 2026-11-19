/*
  ==============================================================================

    HelperFunctions.h
    Created: 6 Jul 2026 2:15:42pm
    Author:  julian

  ==============================================================================
*/

#pragma once

#include <algorithm>
#include <cmath>

#include "stmlib/stmlib.h"
#include "stmlib/dsp/dsp.h"

namespace helper {

    class SineOsc {
    public:
        void prepare(double sample_rate) {
            this->sample_rate = sample_rate;
            phase = 0.0f;
        }

        float getNextValue(float frequency, float phase_offset) {
            float value = std::sin(2.0f * M_PI * (phase + phase_offset));
            phase = std::fmod(phase + (frequency / sample_rate), 1.0f);
            return value;
        }
    private:
        double phase;
        double sample_rate;
    };

    class ValueSmoother {
    public:
        ValueSmoother(float* value_ptr, float step_size, float current_value = 0) :
            value_ptr(value_ptr),
            step_size(step_size),
            current_value(current_value){

        };

        inline void reset(float default_value = 0) {
            current_value = default_value;
        }

        inline operator float() {
            float delta = *value_ptr - current_value;
            if (std::abs(delta) > step_size) {
                if (delta > 0.0f)
                    current_value = std::min(*value_ptr, current_value + step_size);
                else
                    current_value = std::max(*value_ptr, current_value - step_size);
            }
            else
            {
                current_value = *value_ptr;
            }
            return current_value;
        };

    private:
        float* value_ptr;
        float current_value;
        float step_size;
    };

    template<typename T>
    class Ramper {
    public: 
        Ramper() {
            ramping = false;
        }

        void initRamping(T start_value, T duration) {
            this->duration = duration;
            initRamping(start_value);
        }

        void initRamping(T start_value) {
            start = value = start_value;
            ramping = true;
        }

        void stopRamping() {
            ramping = false;
        }

        float ramp(T end_value) {
            end = end_value;
            return ramp();
        }

        operator T() {
            return (T)value;
        }

        T operator=(T other) {
            return value = start = other;
        }

    private:
        T value;
        T start;
        T end;
        T duration;
        bool ramping;

        float ramp() {
            if (ramping) {
                value += ((end - start) / duration);
                if (end - start > 0) {
                    if (value >= end) {
                        value = end;
                        ramping = false;
                    }
                }
                else if (end - start < 0) {
                    if (value <= end) {
                        value = end;
                        ramping = false;
                    }
                }
                else {
                    value = start;
                    ramping = false;
                }
            }
            return value;
        };

    };

    inline float tapeResponse(const float in, float drive, float max_drive_gain, float drive_compensation) {
        return stmlib::SoftClip(stmlib::SoftClip((1.0 + max_drive_gain * drive) * in)
            / (1.0f + drive_compensation * drive));
    };

    template<typename T>
    inline float cosineBlend(T in_1, T in_2, float blend) {
        return std::sin(blend * M_PI / 2.0f) * in_1 + std::cos(blend * M_PI / 2.0f) * in_2;
    };

    inline float clamp(float in, float min, float max){
        if(in < min){
            return min;
        }else if(in > max){
            return max;
        }else{
            return in;
        }
    };

    inline int clamp(int in, int min, int max){
        if(in < min){
            return min;
        }else if(in > max){
            return max;
        }else{
            return in;
        }
    };
}
