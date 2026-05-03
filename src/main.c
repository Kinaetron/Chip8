#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <stdbool.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_begin_code.h>
#include <SDL3/SDL_dialog.h>

#include <cimgui.h>
#include <cimgui_impl.h>

#include <malloc.h>

#include "chip8.h"
#include "renderer.h"


#define igGetIO igGetIO_Nil

typedef struct 
{
	Chip8State* state;
	SDL_Window* window;
	SDL_GPUDevice* gpu_device;
	ImGuiIO* io;
	GraphicsContext* graphicsContext;
} Context;

static const SDL_DialogFileFilter filters[] =
{
	{ "Chip 8 Rom",  "ch8" },
	{ "All files",   "*" }
};

static void SDLCALL callback(void* userdata, const char* const* filelist, int filter)
{
	Context* context = (Context*)userdata;
	chip8_state_initialization(context->state);

	if (!filelist || !*filelist) {
		return;
	}

	const char* filePath = filelist[0];

	if (chip8_load_rom(context->state, filePath)) {
		SDL_Log("ROM Loaded successfully!");
	}
	else {
		SDL_Log("Failed to load ROM!");
	}
}

SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[])
{
	SDL_SetAppMetadata("Chip 8 Emulator", "1.0", "com.chip8-emulator");
	if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
	{
		SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	float main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
	SDL_WindowFlags window_flags = SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY;

	Context* context = calloc(1, sizeof(Context));
	context->state = calloc(1, sizeof(Chip8State));
	context->graphicsContext = calloc(1, sizeof(GraphicsContext));

	context->window = SDL_CreateWindow("Chip 8 Emulator", (int)(640 * main_scale), (int)(320 * main_scale), window_flags);
	if (context->window == NULL)
	{
		SDL_Log("Couldn't create window: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	SDL_SetWindowPosition(context->window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
	SDL_ShowWindow(context->window);

	context->gpu_device = SDL_CreateGPUDevice(
		SDL_GPU_SHADERFORMAT_SPIRV | 
		SDL_GPU_SHADERFORMAT_DXIL | 
		SDL_GPU_SHADERFORMAT_MSL | 
		SDL_GPU_SHADERFORMAT_METALLIB, 
		true, 
		NULL);

	if (context->gpu_device == NULL)
	{
		SDL_Log("Error: SDL_CreateGPUDevice: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	if (!SDL_ClaimWindowForGPUDevice(context->gpu_device, context->window))
	{
		SDL_Log("Error: SDL_CreateGPUDevice Window Claim: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	SDL_SetGPUSwapchainParameters(
		context->gpu_device,
		context->window,
		SDL_GPU_SWAPCHAINCOMPOSITION_SDR, 
		SDL_GPU_PRESENTMODE_VSYNC);

	igCreateContext(NULL);
	context->io = igGetIO(); (void)context->io;

	context->io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	context->io->ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

	igStyleColorsDark(NULL);

	ImGuiStyle* style = igGetStyle();
	ImGuiStyle_ScaleAllSizes(style, main_scale);
	style->FontScaleDpi = main_scale;
	context->io->ConfigDpiScaleFonts = true;
	context->io->ConfigDpiScaleViewports = true;

	ImGui_ImplSDL3_InitForSDLGPU(context->window);
	ImGui_ImplSDLGPU3_InitInfo init_info;

	init_info.Device = context->gpu_device;
	init_info.ColorTargetFormat = SDL_GetGPUSwapchainTextureFormat(
		context->gpu_device,
		context->window);

	init_info.MSAASamples = SDL_GPU_SAMPLECOUNT_1;
	ImGui_ImplSDLGPU3_Init(&init_info);

	chip8_state_initialization(context->state);

	InitializeRenderer(
		context->gpu_device,
		context->window,
		context->graphicsContext);

	*appstate = context;
	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event)
{
	ImGui_ImplSDL3_ProcessEvent(event);

	if (event->type == SDL_EVENT_QUIT) {
		return SDL_APP_SUCCESS;
	}

	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate)
{
	Context* context = (Context*)appstate;

	ImGui_ImplSDLGPU3_NewFrame();
	ImGui_ImplSDL3_NewFrame();
	igNewFrame();

	{
		if (igBeginMainMenuBar())
		{
			if (igBeginMenu("File", true))
			{
				if (igMenuItem_Bool("Open ROM...", NULL, false, true))
				{
					SDL_ShowOpenFileDialog(
						callback,
						context,
						context->window,
						filters,
						SDL_arraysize(filters),
						NULL,
						false
					);
				}

				if (igMenuItem_Bool("Quit", NULL, false, true))
				{
					SDL_Event quit_event;
					quit_event.type = SDL_EVENT_QUIT;
					SDL_PushEvent(&quit_event);
				}

				igEndMenu();
			}

			igEndMainMenuBar();
		}
	}

	igRender();
	ImDrawData* draw_data = igGetDrawData();
	const bool is_minimized = (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);

	SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(context->gpu_device);
	if (commandBuffer == NULL)
	{
		SDL_Log("AcquireGPUCommandBuffer failed: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	UploadChip8Texture(
		context->gpu_device,
		commandBuffer,
		context->graphicsContext,
		context->state->video);


	SDL_GPUTexture* swapchainTexture;
	if (!SDL_WaitAndAcquireGPUSwapchainTexture(
		commandBuffer,
		context->window, 
		&swapchainTexture, 
		NULL, 
		NULL)) 
	{
		SDL_Log("WaitAndAcquireGPUSwapchainTexture failed: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	if (swapchainTexture != NULL)
	{
		ImGui_ImplSDLGPU3_PrepareDrawData(draw_data, commandBuffer);

		SDL_GPUColorTargetInfo target_info = { 0 };
		target_info.texture = swapchainTexture;
		target_info.load_op = SDL_GPU_LOADOP_CLEAR;
		target_info.store_op = SDL_GPU_STOREOP_STORE;
		target_info.clear_color = (SDL_FColor){ 0.0f, 0.0f, 0.0f, 1.0f };
		target_info.load_op = SDL_GPU_LOADOP_CLEAR;
		target_info.store_op = SDL_GPU_STOREOP_STORE;

		SDL_GPURenderPass* renderPass =
			SDL_BeginGPURenderPass(commandBuffer, &target_info, 1, NULL);

		SDL_BindGPUGraphicsPipeline(renderPass, context->graphicsContext->pipeline);
		SDL_BindGPUVertexBuffers(renderPass, 0, &(SDL_GPUBufferBinding){.buffer = context->graphicsContext->vertexBuffer, .offset = 0 }, 1);
		SDL_BindGPUIndexBuffer(renderPass, &(SDL_GPUBufferBinding){.buffer = context->graphicsContext->indexBuffer, .offset = 0 }, SDL_GPU_INDEXELEMENTSIZE_16BIT);
		SDL_BindGPUFragmentSamplers(renderPass, 0, &(SDL_GPUTextureSamplerBinding){.texture = context->graphicsContext->screenTexture, .sampler = context->graphicsContext->sampler }, 1);
		SDL_DrawGPUIndexedPrimitives(renderPass, 6, 1, 0, 0, 0);

		ImGui_ImplSDLGPU3_RenderDrawData(draw_data, commandBuffer, renderPass, NULL);

		SDL_EndGPURenderPass(renderPass);
	}

	SDL_SubmitGPUCommandBuffer(commandBuffer);

	return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) 
{
	Context* context = (Context*)appstate;

	if (context)
	{
		SDL_WaitForGPUIdle(context->gpu_device);
		ImGui_ImplSDL3_Shutdown();
		ImGui_ImplSDLGPU3_Shutdown();
		igDestroyContext(NULL);

		SDL_ReleaseWindowFromGPUDevice(context->gpu_device, context->window);
		SDL_DestroyGPUDevice(context->gpu_device);
		SDL_DestroyWindow(context->window);

		free(context->state);
		free(context);
	}

	SDL_Quit();
}