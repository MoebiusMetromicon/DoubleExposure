/*
  ==============================================================================

    MetaParameter.h
    Created: 20 Jun 2026 11:16:14pm
    Author:  julian

  ==============================================================================
*/

#pragma once

#include <algorithm>
#include <vector>
#include <map>
#include <utility>
#include <cmath>

struct ValueMap {
    float x;
    float y;
};

template<typename metaToSub>
class SubParameter
{
public:
    SubParameter(metaToSub parameter, std::initializer_list<ValueMap> maps) :
        parameter(parameter),
        value_maps(maps),
        target_value(0)
    {
        std::sort(value_maps.begin(), value_maps.end(), [](ValueMap a, ValueMap b) {return a.x < b.x; });

    };

    virtual void reset(size_t buffer_size, float sample_rate) {
        this->buffer_size = buffer_size;
        this->sample_rate = sample_rate;
        int x = 0;
    };

    metaToSub getParameter() {
        return parameter;
    };

    virtual operator float() {
        return target_value;
    };

    virtual float operator= (float metaValue) {
        // Check if metaValue is inside range of valueMaps, extrapolate constantly if not.
        // If metaValue is in range, find the 2 closes points and interpolate with 2-point-form of linear function.

        if (!value_maps.empty()) {
            if (metaValue < value_maps.front().x)
                target_value = value_maps.front().y;
            else if (metaValue >= value_maps.back().x)
                target_value = value_maps.back().y;
            else
                for (int i = 0; i < value_maps.size() - 1; ++i)
                    if (metaValue >= value_maps[i].x && metaValue < value_maps[i + 1].x)
                        target_value = (value_maps[i + 1].y - value_maps[i].y) / ((value_maps[i + 1].x - value_maps[i].x))
                            * (metaValue - value_maps[i].x)
                            + value_maps[i].y;
        }

        return target_value;
    };

    float* getValuePointer() {
        return &target_value;
    };

protected:
    metaToSub parameter;
    float target_value;
    std::vector<ValueMap> value_maps;
    float sample_rate;
    size_t buffer_size;
};

template<typename metaToSub>
class ConstantRateSubParameter : public SubParameter<metaToSub> {
public:
    ConstantRateSubParameter(metaToSub parameter, std::initializer_list<ValueMap> maps, float step_size) :
        SubParameter<metaToSub>(parameter, maps),
        step_size(step_size),
        current_value(this->target_value)
    {
    };

    virtual void reset(size_t buffer_size, float sample_rate) override{
        this->SubParameter<metaToSub>::reset(buffer_size, sample_rate);
        current_value = this->target_value;
    };

    virtual operator float() override{
        float delta = this->target_value - current_value;
        if (std::abs(delta) > step_size) {
            if (delta > 0.0f) {
                current_value = std::min(this->target_value, current_value + step_size);
            }
            else {
                current_value = std::max(this->target_value, current_value - step_size);
            } 
        }
        else
        {
            current_value = this->target_value;
        }
        return current_value;
    };

    virtual float operator= (float metaValue) override {
        this->SubParameter<metaToSub>::operator=(metaValue);
        return current_value;
    };

private:
    float current_value;
    float step_size;
};

template<typename metaToSub>
class ConstantTimeSubParameter : public SubParameter<metaToSub> {
public:
    ConstantTimeSubParameter(metaToSub parameter, std::initializer_list<ValueMap> maps, float step_size) :
        SubParameter<metaToSub>(parameter, maps),
        step_size(step_size),
        current_value(this->target_value)
    {
        delta = 0;
    };

    virtual void reset(size_t buffer_size, float sample_rate) override {
        this->SubParameter<metaToSub>::reset(buffer_size, sample_rate);
        current_value = this->target_value;
        delta = 0;
    };

    virtual operator float() override {
        if ((delta > 0.0f && this->target_value - current_value > delta * step_size) ||
            (delta < 0.0f && this->target_value - current_value < delta * step_size)) {
            current_value = current_value + delta * step_size;
        }
        else
        {
            current_value = this->target_value;
        }
        return current_value;
    };

    virtual float operator= (float metaValue) override {
        this->SubParameter<metaToSub>::operator=(metaValue);
        delta = this->target_value - current_value;
        return current_value;
    };

private:
    float delta;
    float current_value;
    float step_size;
};

template<typename metaToSub>
class InterpolateSubParameter : public SubParameter<metaToSub> {
public:
    InterpolateSubParameter(metaToSub parameter, std::initializer_list<ValueMap> maps, float step_size) :
        SubParameter<metaToSub>(parameter, maps),
        current_value(this->target_value),
        delta(1.0f)
    {

    };

    virtual void reset(size_t buffer_size, float sample_rate) override {
        SubParameter<metaToSub>::reset(buffer_size, sample_rate);
        current_value = this->target_value;
        delta = 0.0f;
    };

    virtual operator float() override {
        if (std::abs(current_value - this->target_value) <= std::abs(delta)) {
            current_value = this->target_value;
        }
        else {
            current_value += delta;
        }
        return current_value;
    };

    virtual float operator= (float metaValue) override {
        this->SubParameter<metaToSub>::operator=(metaValue);
        delta = (this->target_value - current_value) / (float)this->buffer_size;
        return current_value;
    };

protected:
    float delta;
    float current_value;
};

template<typename metaToSub>
class InterpolateLPSubParameter : public InterpolateSubParameter<metaToSub> {
public:
    InterpolateLPSubParameter(metaToSub parameter, std::initializer_list<ValueMap> maps, float step_size) :
        InterpolateSubParameter<metaToSub>(parameter, maps, step_size),
        cutoff_frequency(step_size),
        alpha(1.0f),
        previous_output(this->target_value)
    {

    };

    virtual void reset(size_t buffer_size, float sample_rate) override{
        this->InterpolateSubParameter<metaToSub>::reset(buffer_size, sample_rate);
        previous_output = this->target_value;
        alpha = (2.0f * M_PI * (1.0f / sample_rate) * cutoff_frequency) / (2.0f * M_PI * (1.0f / sample_rate) * cutoff_frequency + 1);
    };

    virtual operator float() override {
        this->InterpolateSubParameter<metaToSub>::operator float();
        previous_output = alpha * this->current_value + (1.0f - alpha) * previous_output;
        return previous_output;
    };

    virtual float operator= (float metaValue) override {
        this->InterpolateSubParameter<metaToSub>::operator=(metaValue);
        return previous_output;
    };

private:
    float alpha;
    float previous_output;
    float cutoff_frequency;
};

enum SubParameterType {
    JUMP,
    CONSTANT_RATE,
    CONSTANT_TIME,
    INTERPOLATE,
    INTERPOLATE_LP
};

template<typename metaToSub>
struct subParameterDefinition {
    metaToSub parameter;
    SubParameterType subParameterType;
    float step_size;
    std::initializer_list<ValueMap> valueMaps;
};

template<typename metaToSub>
class MetaParameter
{
public:
    MetaParameter(std::initializer_list<subParameterDefinition<metaToSub>> subParameters) {
        for (subParameterDefinition<metaToSub> subDef : subParameters) {
            switch (subDef.subParameterType) {
                case JUMP:
                    this->sub_maps[subDef.parameter] = new SubParameter<metaToSub>(subDef.parameter, subDef.valueMaps);
                    break;
                case CONSTANT_RATE:
                    this->sub_maps[subDef.parameter] = new ConstantRateSubParameter<metaToSub>(subDef.parameter, subDef.valueMaps, subDef.step_size);
                    break;
                case CONSTANT_TIME:
                    this->sub_maps[subDef.parameter] = new ConstantTimeSubParameter<metaToSub>(subDef.parameter, subDef.valueMaps, subDef.step_size);
                    break;
                case INTERPOLATE:
                    this->sub_maps[subDef.parameter] = new InterpolateSubParameter<metaToSub>(subDef.parameter, subDef.valueMaps, subDef.step_size);
                    break;
                case INTERPOLATE_LP:
                    this->sub_maps[subDef.parameter] = new InterpolateLPSubParameter<metaToSub>(subDef.parameter, subDef.valueMaps, subDef.step_size);
                    break;
                default:
                    this->sub_maps[subDef.parameter] = new SubParameter<metaToSub>(subDef.parameter, subDef.valueMaps);
                    break;
            }
        }
    };

    ~MetaParameter() {
        for (auto subMap : sub_maps)
            delete(subMap.second);
    };

    void reset(size_t buffer_size, float sample_rate) {
        for (auto sub_map : sub_maps) {
            sub_map.second->reset(buffer_size, sample_rate);
        }
    }

    float operator= (float metaValue) {
        this->metaValue = metaValue;
        for (auto sub_map : sub_maps)
            *(sub_map.second) = metaValue;

        return metaValue;
    };

    float* getSubParameterPointer(metaToSub subParameter) {
        return sub_maps[subParameter]->getValuePointer();
    };

    operator float() const {
        return metaValue;
    };

    float operator[] (metaToSub subParameter) {
        return *sub_maps[subParameter];
    };

private:
    float metaValue;
    std::map<metaToSub, SubParameter<metaToSub>*> sub_maps;
};

