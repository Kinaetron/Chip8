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

#define igGetIO igGetIO_Nil

static SDL_Window* window = NULL;
static SDL_GPUDevice* gpu_device = NULL;

ImGuiIO* io;

static const SDL_DialogFileFilter filters[] =
{
	{ "Chip 8 Rom",  "ch8" },
	{ "All files",   "*" }
};

static void SDLCALL callback(void* userdata, const char* const* filelist, int filter)
{
	if (!filelist)
	{
		SDL_Log("An error occured: %s", SDL_GetError());
		return;
	}
	else if (!*filelist)
	{
		SDL_Log("The user did not select any file.");
		SDL_Log("Most likely, the dialog was canceled.");
		return;
	}

	while (*filelist)
	{
		SDL_Log("Full path to selected file: '%s'", *filelist);
		filelist++;
	}
	if (filter < 0)
	{
		SDL_Log("The current platform does not support fetching "
			"the selected filter, or the user did not select"
			" any filter.");
	}
	else if (filter < SDL_arraysize(filters))
	{
		SDL_Log("The filter selected by the user is '%s' (%s).",
			filters[filter].pattern, filters[filter].name);
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
	SDL_WindowFlags window_flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY;

	window = SDL_CreateWindow("Chip 8 Emulator", (int)(640 * main_scale), (int)(330 * main_scale), window_flags);
	if (window == NULL)
	{
		SDL_Log("Couldn't create window: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
	SDL_ShowWindow(window);

	gpu_device = SDL_CreateGPUDevice(
		SDL_GPU_SHADERFORMAT_SPIRV | 
		SDL_GPU_SHADERFORMAT_DXIL | 
		SDL_GPU_SHADERFORMAT_MSL | 
		SDL_GPU_SHADERFORMAT_METALLIB, 
		true, 
		NULL);

	if (gpu_device == NULL) 
	{
		SDL_Log("Error: SDL_CreateGPUDevice: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	if (!SDL_ClaimWindowForGPUDevice(gpu_device, window))
	{
		SDL_Log("Error: SDL_CreateGPUDevice Window Claim: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	SDL_SetGPUSwapchainParameters(
		gpu_device, 
		window, 
		SDL_GPU_SWAPCHAINCOMPOSITION_SDR, 
		SDL_GPU_PRESENTMODE_VSYNC);

	//IMGUI_CHECKVERSION();
	igCreateContext(NULL);
	io = igGetIO(); (void)io;

	io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io->ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

	igStyleColorsDark(NULL);

	ImGuiStyle* style = igGetStyle();
	ImGuiStyle_ScaleAllSizes(style, main_scale);
	style->FontScaleDpi = main_scale;
	io->ConfigDpiScaleFonts = true;
	io->ConfigDpiScaleViewports = true;

	ImGui_ImplSDL3_InitForSDLGPU(window);
	ImGui_ImplSDLGPU3_InitInfo init_info;

	init_info.Device = gpu_device;
	init_info.ColorTargetFormat = SDL_GetGPUSwapchainTextureFormat(gpu_device, window);
	init_info.MSAASamples = SDL_GPU_SAMPLECOUNT_1;
	ImGui_ImplSDLGPU3_Init(&init_info);

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
						NULL,
						window,
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

	SDL_GPUCommandBuffer* cmdbuf = SDL_AcquireGPUCommandBuffer(gpu_device);
	if (cmdbuf == NULL)
	{
		SDL_Log("AcquireGPUCommandBuffer failed: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	SDL_GPUTexture* swapchainTexture;
	if (!SDL_WaitAndAcquireGPUSwapchainTexture(
		cmdbuf, 
		window, 
		&swapchainTexture, 
		NULL, 
		NULL)) 
	{
		SDL_Log("WaitAndAcquireGPUSwapchainTexture failed: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	if (swapchainTexture != NULL)
	{
		ImGui_ImplSDLGPU3_PrepareDrawData(draw_data, cmdbuf);

		SDL_GPUColorTargetInfo target_info;
		target_info.texture = swapchainTexture;
		target_info.load_op = SDL_GPU_LOADOP_CLEAR;
		target_info.store_op = SDL_GPU_STOREOP_STORE;
		target_info.clear_color = (SDL_FColor){ 0.392f, 0.584f, 0.929f, 1.0f };
		target_info.mip_level = 0;
		target_info.layer_or_depth_plane = 0;
		target_info.cycle = false;
		target_info.resolve_texture = NULL;
		target_info.resolve_mip_level = 0;
		target_info.resolve_layer = 0;
		target_info.cycle_resolve_texture = false;
		target_info.padding1 = 0;
		target_info.padding2 = 0;
		SDL_GPURenderPass* render_pass = SDL_BeginGPURenderPass(cmdbuf, &target_info, 1, NULL);

		ImGui_ImplSDLGPU3_RenderDrawData(draw_data, cmdbuf, render_pass, NULL);

		SDL_EndGPURenderPass(render_pass);
	}

	SDL_SubmitGPUCommandBuffer(cmdbuf);

	return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) 
{
	SDL_WaitForGPUIdle(gpu_device);
	ImGui_ImplSDL3_Shutdown();
	ImGui_ImplSDLGPU3_Shutdown();
	igDestroyContext(NULL);

	SDL_ReleaseWindowFromGPUDevice(gpu_device, window);
	SDL_DestroyGPUDevice(gpu_device);
	SDL_DestroyWindow(window);
	SDL_Quit();
}