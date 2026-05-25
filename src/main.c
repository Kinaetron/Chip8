#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_stdinc.h>

#include <stdint.h>
#include <malloc.h>

#include "ui.h"
#include "context.h"
#include "renderer.h"
#include "chip8.h"

#define CYCLES_PER_FRAME 10
#define MS_PER_FRAME (1000.0 / 60.0)

SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[])
{
	CHIP8_LOAD_ROM_EVENT = SDL_RegisterEvents(1);

	Context* context = calloc(1, sizeof(Context));
	if (context == NULL) 
	{
		SDL_Log("Couldn't allocate memory for context");
		return SDL_APP_FAILURE;
	}

	context->state = calloc(1, sizeof(Chip8State));
	if (context->state == NULL)
	{
		SDL_Log("Couldn't allocate memory for chip8 state");
		return SDL_APP_FAILURE;
	}

	context->graphicsContext = calloc(1, sizeof(GraphicsContext));
	if (context->graphicsContext == NULL)
	{
		SDL_Log("Couldn't allocate memory for graphics context");
		return SDL_APP_FAILURE;
	}

	SDL_AppResult result = InitializeRenderer(context);

	if (result == SDL_APP_FAILURE) {
		return SDL_APP_FAILURE;
	}

	*appstate = context;
	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event)
{
	Context* context = (Context*)appstate;

	chip_input_state(event, context->state->keypad);
	ui_process_event(event);

	if (event->type == CHIP8_LOAD_ROM_EVENT) 
	{
		const char* path = (const char*)event->user.data1;

		if (chip8_load_rom(context->state, path)) {
			SDL_Log("ROM loaded successfully!");
		}
		else {
			SDL_Log("Failed to load ROM!");
		}

		SDL_free(event->user.data1);
		return SDL_APP_CONTINUE;
	}

	if (event->type == SDL_EVENT_QUIT) {
		return SDL_APP_SUCCESS;
	}

	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate)
{
	uint64_t start_time = SDL_GetPerformanceCounter();
	Context* context = (Context*)appstate;

	for (int i = 0; i < CYCLES_PER_FRAME; i++) {
		chip8_cycle(context->state);
	}

	if (context->state->delay_timer > 0) {
		context->state->delay_timer--;
	}
	if (context->state->sound_timer > 0) {
		context->state->sound_timer--;
	}

	SDL_AppResult render_result = Render(
		context->gpu_device,
		context->graphicsContext,
		context->window,
		context->state);

	if (render_result == SDL_APP_FAILURE) {
		return SDL_APP_FAILURE;
	}

	uint64_t end_time = SDL_GetPerformanceCounter();
	double elapsed_ms = (double)(end_time - start_time) * 1000.0 / SDL_GetPerformanceFrequency();

	double remaining_ms = MS_PER_FRAME - elapsed_ms;
	if (remaining_ms > 0) {
		SDL_Delay((uint32_t)remaining_ms);
	}

	return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) 
{
	Context* context = (Context*)appstate;

	if (context) 
	{
		DestroyRenderer(context->gpu_device, context->graphicsContext);

		free(context->graphicsContext);
		SDL_DestroyGPUDevice(context->gpu_device);
		SDL_DestroyWindow(context->window);

		free(context);
	}
}