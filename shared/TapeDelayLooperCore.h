/*
  ==============================================================================

    TapeDelayLooperCore.h
    Created: 19 Jun 2026 1:54:17pm
    Author:  julian

  ==============================================================================
*/

#pragma once

#include <list>
#include <cstdlib>
#include <atomic>

#include "stmlib/stmlib.h"
#include "stmlib/dsp/delay_line.h"
#include "stmlib/utils/buffer_allocator.h"

#include "TapeLine.h"
#include "MetaParameter.h"
#include "SingleSampleVerb.h"
#include "SingleSampleFilter.h"
#include "HelperFunctions.h"

#ifdef ON_DAISY
#include "daisy_seed.h"
#endif 

#define MAX_DELAY_SIZE 96000//2688000
#define MIN_DELAY_RATIO 0.05f
#define MAX_DELAY_TIME_DELTA 0.000010f
#define MAX_VALUE_DELTA 0.00001f
#define FX_ENGINE_BUFFER_SIZE 16384
#define FB_MULT 1.0f
#define MIN_WOW_FREQ 0.25f
#define MAX_WOW_FREQ 12.0f
#define NUM_TAPE_CORES 4
#define DIFFUSION_GAIN_COMPENSATION 0.5f
#define DIFFUSION_FB_COMPENSATION 0.16f
#define DIFFUSION_MAX_MIX 0.6f
#define FLUTTER_BASE_FREQ 2.0f
#define FLUTTER_MAX_FREQ 30.0f
#define RMS_WINDOW_SIZE 16
#define MAX_MOD_AMOUNT 0.01f
#define SLOW_ADD_TIME_MULT 0.5f
#define SLOW_ADD_DURATION sample_rate * 4.0f
#define SLOW_REMOVE_MAX_DELAY_TIME_RATIO 0.9f
#define SLOW_ADD_REMOVE_TRSHLD_SEC 0.8f
#define DELAY_TIME_SCALE 4.0f
#define NOISE_LP_FREQ 300.0f
#define NOISE_HP_FREQ 70.0f
#define NOISE_LP_GAIN 64.0f

#ifndef FLOAT_FRAME_SAMPLE
#define FLOAT_FRAME_SAMPLE(buffer, channel, sample) (channel ? buffer[sample].r : buffer[sample].l)
#endif

using namespace helper;

enum BlendParameter {
	BLEND_DRY_WET,
	BLEND_LOOPER_FB
};

enum DelayParameter {
	DELAY_TIME
};

enum FeedbackParameter {
	FEEDBACK_LEVEL
};

enum FadeParameter {
	FADE_TIME
};

enum CharacterParameter {
	CHARACTER_DRIVE,
	CHARACTER_LOW_PASS,
	CHARACTER_HIGH_PASS,
	CHARACTER_ARTIFACTS,
	CHARACTER_RES
};

enum ModulationParameter {
	MOD_WOW_AMOUNT,
	MOD_WOW_SPEED,
	MOD_FLUTTER_AMOUNT,
	MOD_FLUTTER_SPEED
};

enum SpaceParameter {
	SPACE_DIFFUSION,
	SPACE_BLEND,
	SPACE_FB_SEND,
	SPACE_DRIVE,
	SPACE_TIME
};

struct TapeDelayLooperBuffers {
	uint16_t* fx_engine_buffer;
	float*** tape_core_buffers;
};

struct TapeDelayLooperCoreParameters {
	MetaParameter<BlendParameter> blend = MetaParameter<BlendParameter>({
		{BLEND_DRY_WET, INTERPOLATE, 0.0f, {{0.0f, 0.0f}, {1.0f, 1.0f}}},
		{BLEND_LOOPER_FB, INTERPOLATE, 0.0f, {{0.0f, 0.0f}, {0.5f, 0.0f}, {1.0f, 1.0f}}},
	});
	MetaParameter<DelayParameter> delay = MetaParameter<DelayParameter>({
		{DELAY_TIME, INTERPOLATE_LP, 1.0f, {{0.0f, 0.0f}, {1.0f, 1.0f}}}
	});
	MetaParameter<FeedbackParameter> feedback = MetaParameter<FeedbackParameter>({
		{FEEDBACK_LEVEL, INTERPOLATE, 0.0f, {{0.0f, 0.0f}, {1.0f, 1.0f}}}
	});
	MetaParameter<FadeParameter> fade = MetaParameter<FadeParameter>({
		{FADE_TIME, JUMP, 10.0f, {{0.0f, 0.0f}, {1.0f, 1.0f}}}
	});
	MetaParameter<SpaceParameter> space = MetaParameter<SpaceParameter>({
		{SPACE_DIFFUSION, INTERPOLATE_LP, 2.0f, {{0.0f, 0.05f}, {0.4f, 0.25f}, {0.5f, 0.5f}, {0.6f, 0.75f}, {1.0f, 0.95f}}},
		{SPACE_BLEND, INTERPOLATE, 0.0f, {{0.0f, 0.85f}, {0.3f, 0.6f}, { 0.5f, 0.0f }, {0.7f, 0.4f}, { 1.0f, 0.6f }}},
		{SPACE_FB_SEND, INTERPOLATE, 0.0f, {{0.0f, 0.6f}, {0.3f, 0.0f}, { 0.7f, 0.0f }, {1.0f, 0.35f}}},
		{SPACE_DRIVE, INTERPOLATE , 0.0f, {{0.0f, 0.5f}, {0.3f, 0.0f}, {1.0f, 0.0f}}},
		{SPACE_TIME, INTERPOLATE_LP, 2.0f, {{0.0f, 0.5f}, {0.3f, 0.2f}, {0.5f, 0.5f}, {1.0f, 0.9f}}}
	});
	MetaParameter<ModulationParameter> modulation = MetaParameter<ModulationParameter>({
		{MOD_WOW_AMOUNT, INTERPOLATE, 0.0f,		{{0.0f, 0.05f}, {0.1f, 0.1f}, {0.25f, 1.0f}, {0.5f, 0.0f}, {0.75f, 0.3f}, {1.0f, 0.0f}}},
		{MOD_WOW_SPEED, INTERPOLATE_LP, 1.0f,	{{0.0f, 1.0f}, {0.25f, 0.0f}, {0.5f, 0.0f}, {0.75f, 0.0f}, {1.0f, 0.0f}}},
		{MOD_FLUTTER_AMOUNT, INTERPOLATE, 0.0f,	{{0.0f, 0.0f}, {0.25f, 0.0f}, {0.5f, 0.0f}, {0.75f, 0.7f}, {1.0f, 1.0f}}},
		{MOD_FLUTTER_SPEED,	INTERPOLATE_LP, 10.0f,	{{0.0f, 0.0f}, {0.25f, 0.0f}, {0.5f, 0.0f}, {0.75f, 0.2f}, {1.0f, 1.0f}}}
	});
	MetaParameter<CharacterParameter> character = MetaParameter<CharacterParameter>({
		{CHARACTER_DRIVE, INTERPOLATE, 0.0f,		{{0.0f, 1.0f}, {0.25f, 0.2f}, {0.5f, 0.0f}, {0.6f, 0.2f},  {1.0f, 0.2f}}},
		{CHARACTER_LOW_PASS, INTERPOLATE, 0.0f,	{{0.0f, 1.0f}, {0.25f, 0.7f}, {0.5f, 0.0f}, {1.0f, 0.0f}}},
		{CHARACTER_HIGH_PASS, INTERPOLATE, 0.0f,	{{0.0f, 0.0f}, {0.5f, 0.0f}, {0.75f, 0.6f}, {1.0f, 1.0}}},
		{CHARACTER_ARTIFACTS, INTERPOLATE, 0.0f,	{{0.0f, 0.0f}, {0.5f, 0.0f}, {0.75, 0.2}, {1.0f, 1.0f}}},
		{CHARACTER_RES, INTERPOLATE, 0.0f,	{{0.0f, 0.2f}, {0.15f, 0.3f}, {0.5f, 0.0f}, {0.6f, 0.2f}, {0.8f, 0.5f}, {1.0f, 0.2f}}}
	});
	bool repitch_loop;
	bool loop_fluctuation;
	bool ping_pong;
	bool remove_oldest_loop_first;
};

class TapeDelayLooperCore
{
public:
	TapeDelayLooperCoreParameters parameters;
	bool add_loop_ctrl;
	bool remove_loop_ctrl;

	TapeDelayLooperCore() :
		sample_rate(48000.0),
		parameters(),
		tape_cores(),
		reverb(),
		active_delay_core(0),
		slow_mod_osc()
	{
		core_out_l = (float*)malloc(NUM_TAPE_CORES * sizeof(float));
		core_out_r = (float*)malloc(NUM_TAPE_CORES * sizeof(float));

		core_rms = (float*)malloc(NUM_TAPE_CORES * sizeof(float*));

		core_rms_window = (float**)malloc(NUM_TAPE_CORES * sizeof(float**));
		for (int i = 0; i < NUM_TAPE_CORES; ++i) {
			core_rms_window[i] = (float*)malloc(RMS_WINDOW_SIZE * sizeof(float*));
			for(int j = 0; j < RMS_WINDOW_SIZE; ++j)
				core_rms_window[i][j] = 0.0f;
		}

		rms_window_ptr = 0;
	}

	~TapeDelayLooperCore() {
		free(core_out_l);
		free(core_out_r);

		free(core_rms);

		for (int i = 0; i < NUM_TAPE_CORES; ++i)
			free(core_rms_window[i]);
		free(core_rms_window);
	}

	void process(
		const float* in_buffer_l,
		const float* in_buffer_r,
		float* out_buffer_l,
		float* out_buffer_r,
		size_t buffer_size,
		size_t channel_count
	) {	
		for (size_t sample = 0; sample < buffer_size; ++sample)
		{
			// read smooth delay time only once per sample
			current_delay_time_ratio = parameters.delay[DELAY_TIME];
			float fade_time = parameters.fade[FADE_TIME];

			// Logic for adding / removing loops
			int hold_thrshld = (int)std::round(sample_rate * SLOW_ADD_REMOVE_TRSHLD_SEC);
			if (add_loop_ctrl) {
				if (time_counter_adding >= 0 && time_counter_adding < hold_thrshld) {
					time_counter_adding++;
				}
				else if (time_counter_adding == hold_thrshld) {
					addLoopSlow(current_delay_time_ratio);
					time_counter_adding++;
				}
			}
			else if (!add_loop_ctrl && old_add_loop) {
				if (time_counter_adding >= hold_thrshld) {
					addLoopAfterSlow(current_delay_time_ratio, fade_time);
				}
				else {
					addLoop(current_delay_time_ratio, fade_time);
				}
				time_counter_adding = 0;
			}

			old_add_loop = add_loop_ctrl;

			if (remove_loop_ctrl) {
				if (time_counter_removing >= 0 && time_counter_removing < hold_thrshld) {
					time_counter_removing++;
				}
				else if (time_counter_removing == hold_thrshld) {
					removeLoopSlow();
					time_counter_removing++;
				}
			}

			else if (!remove_loop_ctrl && old_remove_loop) {
				time_counter_removing = 0;
				removeLoop();
			}

			old_remove_loop = remove_loop_ctrl;

			out_buffer_l[sample] = 0.0f;
			out_buffer_r[sample] = 0.0f;

			// calc modulation
			float wow = slow_mod_osc.getNextValue(MIN_WOW_FREQ + valueScaler(parameters.modulation[MOD_WOW_SPEED], 6.0f) * (MAX_WOW_FREQ - MIN_WOW_FREQ), 0.0);
			flutter_filter.set_f_q<SingleSampleFilter::FREQUENCY_FAST>((FLUTTER_BASE_FREQ + FLUTTER_MAX_FREQ * parameters.modulation[MOD_FLUTTER_SPEED]) / sample_rate, 0.5f);
			float flutter = (1.0f + (1.0f - parameters.modulation[MOD_FLUTTER_SPEED])) * flutter_filter.Process<SingleSampleFilter::FILTER_MODE_LOW_PASS>(((float)std::rand() / ((float)RAND_MAX / 2.0f) - 1.0f) * 0.9);
			float modulation = wow * parameters.modulation[MOD_WOW_AMOUNT] + flutter * parameters.modulation[MOD_FLUTTER_AMOUNT];
			float time_with_mod_l = getEffectiveDelayRatio(current_delay_time_ratio, modulation);
			float time_with_mod_r = getEffectiveDelayRatio(current_delay_time_ratio, parameters.ping_pong ? -modulation : modulation);

			float noise = std::rand() / ((float)RAND_MAX / 2.0f) - 1.0f;
			noise = noise_hp.Process<SingleSampleFilter::FILTER_MODE_HIGH_PASS>(noise);
			noise = NOISE_LP_GAIN * noise_lp.Process<SingleSampleFilter::FILTER_MODE_LOW_PASS>(noise) * audio_mod_lp.Process<SingleSampleFilter::FILTER_MODE_LOW_PASS>(std::abs(in_buffer_l[sample]));

			float looper_out_l = 0.0f;
			float looper_out_r = 0.0f;

			for (size_t core = 1; core < NUM_TAPE_CORES; ++core) {
				switch (tape_cores[core].state) {
					case LOOPER: {
						core_gain[core].ramp(1.0f); // fade-in loop slowly

						tape_cores[core].readLoop(
							core_out_l[core],
							core_out_r[core],
							time_with_mod_l,
							time_with_mod_r,
							fade_time,
							parameters.repitch_loop,
							parameters.loop_fluctuation,
							parameters.ping_pong
						);
					} break;

					case ADDING: {
						tape_cores[core].readInterpolated(
							core_out_l[core],
							core_out_r[core],
							getEffectiveDelayRatio(current_delay_time_ratio / slow_add_delay_time_factor),
							getEffectiveDelayRatio(current_delay_time_ratio / slow_add_delay_time_factor)
						);

						tape_cores[core].writeWithDrive(
							core_out_l[core],
							core_out_r[core],
							parameters.character[CHARACTER_DRIVE] + slow_add_drive,
							parameters.character[CHARACTER_HIGH_PASS],
							parameters.character[CHARACTER_LOW_PASS],
							parameters.character[CHARACTER_RES]
						);

						if (parameters.repitch_loop) {
							slow_add_delay_time_factor.ramp(2.0f);
						}
						slow_add_drive.ramp(1.0f - parameters.character[CHARACTER_DRIVE]);
						core_gain[core].ramp(1.0f);
					} break;

					case REMOVING: {
						tape_cores[core].readLoop(
							core_out_l[core],
							core_out_r[core],
							time_with_mod_l * slow_remove_delay_time_factor,
							time_with_mod_r * slow_remove_delay_time_factor,
							fade_time,
							parameters.repitch_loop,
							parameters.loop_fluctuation,
							parameters.ping_pong
						);

						if (parameters.repitch_loop) {
							slow_remove_delay_time_factor.ramp(2.0f);
						}
						slow_remove_filter.ramp(1.0f);
						core_gain[core].ramp(0.0f);
					} break;

					case IDLE: {
						core_gain[core].ramp(0.0f);
						if (core_gain[core] > 0.0f) {
							tape_cores[core].readLoop(
								core_out_l[core],
								core_out_r[core],
								parameters.repitch_loop ? time_with_mod_l : 1.0f,
								parameters.repitch_loop ? time_with_mod_r : 1.0f,
								fade_time,
								parameters.repitch_loop,
								parameters.loop_fluctuation,
								parameters.ping_pong
							);
						}
					}break;

					default: {
						core_out_l[core] = 0.0f;
						core_out_r[core] = 0.0f;
					}break;
				}

				looper_out_l += core_out_l[core] * core_gain[core];
				looper_out_r += core_out_r[core] * core_gain[core];
			}

			for (size_t core = 0; core < NUM_TAPE_CORES; ++core) {
				switch (tape_cores[core].state) {
					case DELAY: {
						// Read from tape delay line
						tape_cores[core].readInterpolated(
							core_out_l[core],
							core_out_r[core],
							getEffectiveDelayRatio(current_delay_time_ratio, modulation),
							getEffectiveDelayRatio(current_delay_time_ratio, parameters.ping_pong ? -modulation : modulation)
						);

						setReverbParameters();

						float send_l = in_buffer_l[sample], send_r = in_buffer_r[sample];
						float return_l, return_r;

						crossfadeStereo(core_out_l[core], core_out_r[core], send_l, send_r, 1.0f - parameters.blend[BLEND_DRY_WET]); // blend input with delay
						crossfadeStereo(
							helper::tapeResponse(send_l, parameters.space[SPACE_DRIVE], MAX_DRIVE_GAIN, DRIVE_COMPENSATION), 
							helper::tapeResponse(send_r, parameters.space[SPACE_DRIVE], MAX_DRIVE_GAIN, DRIVE_COMPENSATION),
							send_l,
							send_r,
							parameters.space[SPACE_DRIVE]
						); // apply drive
						send_l += parameters.blend[BLEND_LOOPER_FB] * looper_out_l;
						send_r += parameters.blend[BLEND_LOOPER_FB] * looper_out_r;
						reverb.ProcessSingleSample(send_l, send_r, return_l, return_r);

						// Filter the feedback signal
						float fb_l = parameters.feedback[FEEDBACK_LEVEL] * FB_MULT * ((1.0f - parameters.space[SPACE_FB_SEND]) * core_out_l[core]
							+ parameters.space[SPACE_FB_SEND] * return_l);
						float fb_r = parameters.feedback[FEEDBACK_LEVEL] * FB_MULT * ((1.0f - parameters.space[SPACE_FB_SEND]) * core_out_r[core] 
							+ parameters.space[SPACE_FB_SEND] * return_r);
						float cutoff = (20.0f + 100.0f * parameters.feedback[FEEDBACK_LEVEL] * parameters.feedback[FEEDBACK_LEVEL]) / sample_rate;
						fb_filter_l.set_f_q<SingleSampleFilter::FREQUENCY_FAST>(cutoff, 1.0f);
						fb_filter_r.set(fb_filter_l);
						fb_l = fb_filter_l.Process<SingleSampleFilter::FILTER_MODE_HIGH_PASS>(fb_l);
						fb_r = fb_filter_l.Process<SingleSampleFilter::FILTER_MODE_HIGH_PASS>(fb_r);
						fb_l += parameters.blend[BLEND_LOOPER_FB] * looper_out_l;
						fb_r += parameters.blend[BLEND_LOOPER_FB] * looper_out_r;

						// write samples to delay line
						tape_cores[core].writeWithDrive(
							in_buffer_l[sample] + (parameters.ping_pong ? fb_r : fb_l),
							in_buffer_r[sample] + (parameters.ping_pong ? fb_l : fb_r),
							parameters.character[CHARACTER_DRIVE],
							parameters.character[CHARACTER_HIGH_PASS],
							parameters.character[CHARACTER_LOW_PASS],
							parameters.character[CHARACTER_RES],
							noise * parameters.character[CHARACTER_ARTIFACTS],
							noise * parameters.character[CHARACTER_ARTIFACTS]
						);

						crossfadeStereo(return_l, return_r, core_out_l[core], core_out_r[core], parameters.space[SPACE_BLEND]);

					} break;

					case IDLE: {
						if (!core_gain[core] > 0.0f) {
							core_out_l[core] = core_out_r[core] = 0.0f;
							// write current output from active delay line to idle looper lines
							tape_cores[core].write(core_out_l[active_delay_core], core_out_r[active_delay_core]);
						}
					} break;

					default: {

					} break;
				}

				// sum all cores
				out_buffer_l[sample] += core_out_l[core] * core_gain[core];
				out_buffer_r[sample] += core_out_r[core] * core_gain[core];

				core_rms_window[core][rms_window_ptr] = std::max(core_out_l[core], core_out_r[core]) * core_gain[core];
				float mean_square = 0.0f;
				for (int i = 0; i < RMS_WINDOW_SIZE; ++i)
					mean_square += std::pow(core_rms_window[core][i], 2);
				mean_square /= (float)RMS_WINDOW_SIZE;

				#pragma atomic
				core_rms[core] = std::sqrt(mean_square);

				rms_window_ptr = (rms_window_ptr + 1) % RMS_WINDOW_SIZE;
			}
		}

		reverb.afterProcessSingleSample();
	}

	void prepare(size_t buffer_size, size_t num_channels, float sample_rate, TapeDelayLooperBuffers& buffers
		#ifdef ON_DAISY
		, daisy::DaisySeed* hw 
		#endif
	) {
		#ifdef ON_DAISY
		this->hw = hw;
		#endif

		idle_loop_cue.clear();
		active_loop_cue.clear();

		rms_window_ptr = 0;

		// reset reverb
		fx_engine_buffer = buffers.fx_engine_buffer;
		reverb.Init(fx_engine_buffer, sample_rate);

		// reset filter
		fb_filter_l.Init();
		fb_filter_r.Init();
		flutter_filter.Init();

		// reset parametes
		slow_add_delay_time_factor = 1.0f;
		slow_add_drive = 0.0f;
		slow_add_gain = 0.0f;
		slow_remove_gain = 0.0f;
		slow_remove_filter = 0.0f;
		slow_remove_delay_time_factor = 1.0f;
		time_counter_adding = 0;
		time_counter_removing = 0;
		old_add_loop = 0;
		old_remove_loop = 0;

		// reset meta parameters
		parameters.blend.reset(buffer_size, sample_rate);
		parameters.delay.reset(buffer_size, sample_rate);
		parameters.feedback.reset(buffer_size, sample_rate);
		parameters.modulation.reset(buffer_size, sample_rate);
		parameters.character.reset(buffer_size, sample_rate);
		parameters.space.reset(buffer_size, sample_rate);
		parameters.fade.reset(buffer_size, sample_rate);

		this->sample_rate = sample_rate;
		slow_mod_osc.prepare(sample_rate);

		noise_lp.set_f_q<SingleSampleFilter::FREQUENCY_FAST>(NOISE_LP_FREQ / sample_rate, 0.2f);
		noise_hp.set_f_q<SingleSampleFilter::FREQUENCY_FAST>(NOISE_HP_FREQ / sample_rate, 0.5f);
		audio_mod_lp.set_f_q<SingleSampleFilter::FREQUENCY_FAST>(NOISE_LP_FREQ / sample_rate, 0.2f);

		// init tape cores
		for (size_t core = 0; core < NUM_TAPE_CORES; ++core) {
			for (int sample = 0; sample < RMS_WINDOW_SIZE; ++sample) {
				core_rms_window[core][sample] = 0.0f;
			}
			tape_cores[core].reset(sample_rate, buffers.tape_core_buffers[core][0], buffers.tape_core_buffers[core][1]);
			core_out_l[core] = core_out_r[core] = 0.0f;
			fade_core_gain[core] = 0;
			if (core == active_delay_core) {
				tape_cores[core].state = DELAY;
				core_gain[core] = 1.0f;
			}
			else {
				tape_cores[core].state = IDLE;
				core_gain[core] = 0.0f;
				idle_loop_cue.push_front(core);
			}
			idle_loop_cue.sort();
		}
	}

	void setParameters(
		float blend,
		float delay, 
		float feedback, 
		float modulation, 
		float character, 
		float space, 
		float fade,
		bool repitch_loop,
		bool loop_fluctuation,
		bool ping_pong,
		bool remove_oldest_loop_first

	) {
		parameters.blend = blend;
		parameters.delay = delay;
		parameters.feedback = feedback;
		parameters.modulation = modulation;
		parameters.character = character;
		parameters.space = space;
		parameters.fade = fade;
		if (repitch_loop && !parameters.repitch_loop) {
			updateLooperDelayRatio();
		}
		parameters.repitch_loop = repitch_loop;
		parameters.loop_fluctuation = loop_fluctuation;
		parameters.ping_pong = ping_pong;
		parameters.remove_oldest_loop_first = remove_oldest_loop_first;
	}

	const float* getCoreRMS() const {
		return core_rms;
	}

private:
	StereoTapeCore<float, MAX_DELAY_SIZE> tape_cores[NUM_TAPE_CORES];
	uint16_t* fx_engine_buffer;
	SingleSampleVerb reverb;
	SingleSampleFilter::Svf fb_filter_l;
	SingleSampleFilter::Svf fb_filter_r;
	SingleSampleFilter::Svf flutter_filter;
	SingleSampleFilter::Svf noise_lp;
	SingleSampleFilter::Svf audio_mod_lp;
	SingleSampleFilter::Svf noise_hp;
	double sample_rate;
	helper::SineOsc slow_mod_osc;
	size_t active_delay_core;
	Ramper<float> core_gain[NUM_TAPE_CORES];
	bool fade_core_gain[NUM_TAPE_CORES];
	std::list<size_t> idle_loop_cue;
	std::list<size_t> active_loop_cue;
	float* core_out_l;
	float* core_out_r;
	float** core_rms_window;
	std::atomic<float*> core_rms;
	size_t rms_window_ptr;
	Ramper<float> slow_add_delay_time_factor;
	Ramper<float> slow_add_drive;
	Ramper<float> slow_add_gain;
	Ramper<float> slow_remove_delay_time_factor;
	Ramper<float> slow_remove_gain;
	Ramper<float> slow_remove_filter;
	int time_counter_adding;
	int time_counter_removing;
	bool old_add_loop;
	bool old_remove_loop;
	float current_delay_time_ratio;

	#ifdef ON_DAISY
	daisy::DaisySeed* hw;
	#endif

	void addLoop(float current_delay_time_ratio, float fade_time) {
		slow_add_delay_time_factor.stopRamping();
		slow_add_drive.stopRamping();
		if (!idle_loop_cue.empty()) {
			int core = idle_loop_cue.front();
			idle_loop_cue.pop_front();
			active_loop_cue.push_front(core);
			tape_cores[core].setLoopPoint(getEffectiveDelayRatio(current_delay_time_ratio), fade_time, parameters.ping_pong);
			core_gain[core].initRamping(core_gain[core], sample_rate * 0.1f);
		}
	}

	void addLoopAfterSlow(float current_delay_ratio, float fade_time) {
		slow_add_delay_time_factor.stopRamping();
		slow_add_drive.stopRamping();
		if (!idle_loop_cue.empty()) {
			int core = idle_loop_cue.front();
			idle_loop_cue.pop_front();
			active_loop_cue.push_front(core);
			tape_cores[core].setLoopPoint(getEffectiveDelayRatio(current_delay_ratio / slow_add_delay_time_factor), fade_time, parameters.ping_pong);
			tape_cores[core].setInitialDelayRatio(getEffectiveDelayRatio(current_delay_ratio));
			core_gain[core].stopRamping();
		}
	}

	void addLoopSlow(float initial_delay_ratio) {
		if (!idle_loop_cue.empty()) {
			int core = idle_loop_cue.front();
			slow_add_delay_time_factor.initRamping(1.0f, SLOW_ADD_DURATION);
			slow_add_drive.initRamping(0.0f, SLOW_ADD_DURATION);
			core_gain[core].initRamping(0.0f, sample_rate);
			tape_cores[core].state = ADDING;
		}
	}

	void removeLoop() {
		slow_remove_delay_time_factor.stopRamping();
		slow_remove_filter.stopRamping();
		if (!active_loop_cue.empty()) {
			int core = -1;
			if (parameters.remove_oldest_loop_first) {
				core = active_loop_cue.back();
				active_loop_cue.pop_back();
			}
			else {
				core = active_loop_cue.front();
				active_loop_cue.pop_front();
			}
			idle_loop_cue.push_front(core);
			idle_loop_cue.sort();
			tape_cores[core].removeLoop();
			core_gain[core].initRamping(core_gain[core], std::floor(MAX_DELAY_SIZE * MIN_DELAY_RATIO));
		}
	}

	void removeLoopSlow() {
		slow_remove_delay_time_factor.initRamping(1.0f, SLOW_ADD_DURATION);
		slow_remove_filter.initRamping(0.0f, SLOW_ADD_DURATION);
		if (!active_loop_cue.empty()) {
			int core = -1;
			if (parameters.remove_oldest_loop_first) {
				core = active_loop_cue.back();
			}
			else {
				core = active_loop_cue.front();
			}
			tape_cores[core].state = REMOVING;
			core_gain[core].initRamping(core_gain[core], SLOW_ADD_DURATION);
		}
	}

	void setReverbParameters() {
		reverb.set_diffusion(parameters.space[SPACE_DIFFUSION]);
		reverb.set_time(parameters.space[SPACE_TIME]);
		reverb.set_input_gain(DIFFUSION_GAIN_COMPENSATION);
		reverb.set_lp(0.9f);
	};

	void crossfadeStereo(const float& in_1_l, const float& in_1_r, float& in_2_out_l, float& in_2_out_r, float mix) {
		in_2_out_l = in_2_out_l * (1.0f - mix) + in_1_l * mix;
		in_2_out_r = in_2_out_r * (1.0f - mix) + in_1_r * mix;
	}

	float getEffectiveDelayRatio(float delay_ratio, float mod_value = 0.0f) {
		return clamp(
			(valueScaler(delay_ratio, DELAY_TIME_SCALE) * ((1.0f - MAX_MOD_AMOUNT * 2.0f) + MAX_MOD_AMOUNT) + (MAX_MOD_AMOUNT * mod_value)) 
				* (1.0f - MIN_DELAY_RATIO) + MIN_DELAY_RATIO, 
			MIN_DELAY_RATIO, 
			1.0f);
	}

	float valueScaler(float value, float scale) {
		if (value > 0.0f) {
			return (std::pow(2.0f, scale * value) - 1.0f) / (std::pow(2.0f, scale) - 1.0f);
		}
		else if (value < 0.0f){
			return -(std::pow(2.0f, scale * (-value)) - 1.0f) / (std::pow(2.0f, scale) - 1.0f);
		}
		else {
			return 0.0f;
		}
	};

	float rampValueWithin(float& value, float start, float end, float duration) {
		value += ((end - start) / duration);
		if (end - start > 0) {
			if (value >= end) {
				value = end;
			}
		}
		else if (end - start < 0) {
			if (value <= end) {
				value = end;
			}
		}
		else {
			value = start;
		}
		return value;
	}

	void updateLooperDelayRatio() {
		for (auto& core : tape_cores) {
			if (core.state == LOOPER) {
				core.updateInitialDelayRatio(getEffectiveDelayRatio(current_delay_time_ratio));
			}
		}
	}

};
 

