#ifndef AUDIO_H
#define AUDIO_H

#include <SDL3/SDL.h>
#include <stdbool.h>

#define SAMPLE_RATE 44100
#define SAMPLES_PER_FRAME 735
#define WAVE_TABLE_SIZE 256
#define PI 3.14159265358979323846
#define BEEP_FREQUENCY 440.0f


typedef struct AudioContext {
	SDL_AudioStream* stream;
	float wave_table[WAVE_TABLE_SIZE];
	float samples[SAMPLES_PER_FRAME];
	float phase;
	bool playing;
} AudioContext;

bool audio_initialize(AudioContext* audio);
void audio_update(AudioContext* audio, bool should_play);
void audio_destroy(AudioContext* audio);

#endif