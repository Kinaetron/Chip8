#include "audio.h"
#include <math.h>
#include <SDL3/SDL_stdinc.h>

static void audio_callback(void* userdata, SDL_AudioStream* stream, int additional_amount, int total_amount)
{
	AudioContext* audio = (AudioContext*)userdata;

	int samples_needed = additional_amount / sizeof(float);

	float* buffer = SDL_stack_alloc(float, samples_needed);

	if (!buffer) {
		return;
	}

	float phase_increment = BEEP_FREQUENCY / (float)SAMPLE_RATE;

	for (int i = 0; i < samples_needed; i++)
	{
		int index = (int)(audio->phase * WAVE_TABLE_SIZE) % WAVE_TABLE_SIZE;
		buffer[i] = audio->playing ? audio->wave_table[index] : 0.0f;
		audio->phase += phase_increment;

		if (audio->phase >= 1.0f) {
			audio->phase -= 1.0f;
		}
	}

	SDL_PutAudioStreamData(stream, buffer, additional_amount);
	SDL_stack_free(buffer);
}

bool audio_initialize(AudioContext* audio)
{
	SDL_AudioSpec spec = {0};
	spec.format = SDL_AUDIO_F32;
	spec.channels = 1;
	spec.freq = SAMPLE_RATE;

	audio->stream = SDL_OpenAudioDeviceStream(
		SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
		&spec,
		audio_callback,
		audio);

	if (!audio->stream) 
	{
		SDL_Log("Failed to open audio device: %s", SDL_GetError());
		return false;
	}

	for (int i = 0; i < WAVE_TABLE_SIZE; i++) {
		audio->wave_table[i] = 0.3f * sinf(2.0f * PI * i / WAVE_TABLE_SIZE);
	}

	audio->phase = 0.0f;
	audio->playing = false;
	SDL_ResumeAudioStreamDevice(audio->stream);
	return true;
}

void audio_update(AudioContext* audio, bool should_play) {
	audio->playing = should_play;
}

void audio_destroy(AudioContext* audio)
{
	if (audio->stream) {
		SDL_DestroyAudioStream(audio->stream);
	}
}