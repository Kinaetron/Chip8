#ifndef CONTEXT_H
#define CONTEXT_H

#include "chip8.h"
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_gpu.h>

typedef struct GraphicsContext GraphicsContext;

typedef struct Context
{
	Chip8State* state;
	SDL_Window* window;
	SDL_GPUDevice* gpu_device;
	GraphicsContext* graphicsContext;
} Context;

#endif