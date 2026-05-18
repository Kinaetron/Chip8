#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_events.h>

#include <malloc.h>

#include "ui.h"
#include "context.h"
#include "renderer.h"
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_video.h>

SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[])
{
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

	if (event->type == SDL_EVENT_QUIT) {
		return SDL_APP_SUCCESS;
	}

	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate)
{
	Context* context = (Context*)appstate;

	chip8_cycle(context->state);

	return Render(
		context->gpu_device,
		context->graphicsContext,
		context->window,
		context->state);
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