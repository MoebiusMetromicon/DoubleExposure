#include "daisy_seed.h"
#include "daisysp.h"
#include "TapeDelayLooperCore.h"

using namespace daisy;
using namespace daisysp;

DaisySeed hw;
TapeDelayLooperCore core;
uint16_t DSY_SDRAM_BSS fx_engine_buffer[FX_ENGINE_BUFFER_SIZE];
float DSY_SDRAM_BSS tape_buffer_0_l[MAX_DELAY_SIZE];
float DSY_SDRAM_BSS tape_buffer_1_l[MAX_DELAY_SIZE];
float DSY_SDRAM_BSS tape_buffer_2_l[MAX_DELAY_SIZE];
float DSY_SDRAM_BSS tape_buffer_3_l[MAX_DELAY_SIZE];
float DSY_SDRAM_BSS tape_buffer_0_r[MAX_DELAY_SIZE];
float DSY_SDRAM_BSS tape_buffer_1_r[MAX_DELAY_SIZE];
float DSY_SDRAM_BSS tape_buffer_2_r[MAX_DELAY_SIZE];
float DSY_SDRAM_BSS tape_buffer_3_r[MAX_DELAY_SIZE];

CpuLoadMeter load_meter;

float* tape_buffers_0[] = {
	tape_buffer_0_l,
	tape_buffer_0_r
};

float* tape_buffers_1[] = {
	tape_buffer_1_l,
	tape_buffer_1_r
};

float* tape_buffers_2[] = {
	tape_buffer_2_l,
	tape_buffer_2_r
};

float* tape_buffers_3[] = {
	tape_buffer_3_l,
	tape_buffer_3_r
};

float** tape_buffers[] = {
	tape_buffers_0,
	tape_buffers_1,
	tape_buffers_2, 
	tape_buffers_3
};

TapeDelayLooperBuffers buffers = {
	fx_engine_buffer,
	tape_buffers
};

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
	load_meter.OnBlockStart();
	//core.process(in[0], in[1], out[0], out[1], size, 2);
	for(int sample = 0; sample < size; ++sample){
		out[0][sample] = in[0][sample];
		out[1][sample] = in[1][sample];
	}
	load_meter.OnBlockEnd();
}

int main(void)
{
	hw.Configure();
	hw.Init();
	hw.StartLog();
	load_meter.Init(hw.AudioSampleRate(), hw.AudioBlockSize());
	hw.SetAudioBlockSize(4); // number of samples handled per callback
	hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
	System::Delay(3000);
	core.prepare(hw.AudioBlockSize(), 2, hw.AudioSampleRate(), buffers
		#ifdef ON_DAISY
		, &hw
		#endif
	);
	core.setParameters(0.5f, 0.5f, 0.2f, 0.2f, 0.5f, 0.5f, 0.0f, 0, 0, 0, 0);
	hw.StartAudio(AudioCallback);

	while(1) {

		for(int i = 0; i < 5; ++i){
            hw.SetLed(1);
            System::Delay(100);
            hw.SetLed(0);
            System::Delay(100);
        }

		// get the current load (smoothed value and peak values)
        const float avgLoad = load_meter.GetAvgCpuLoad();
        const float maxLoad = load_meter.GetMaxCpuLoad();
        const float minLoad = load_meter.GetMinCpuLoad();
        // print it to the serial connection (as percentages)
        hw.PrintLine("Processing Load %:");
        hw.PrintLine("Max: " FLT_FMT3, FLT_VAR3(maxLoad * 100.0f));
        hw.PrintLine("Avg: " FLT_FMT3, FLT_VAR3(avgLoad * 100.0f));
        hw.PrintLine("Min: " FLT_FMT3, FLT_VAR3(minLoad * 100.0f));

        System::Delay(2000);

	}
}
