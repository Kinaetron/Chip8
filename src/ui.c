#include <SDL3/SDL.h>

#include "ui.h"
#include "chip8.h"

static const SDL_DialogFileFilter filters[] =
{
	{ "Chip 8 Rom",  "ch8" },
	{ "All files",   "*" }
};

static void SDLCALL callback(void* userdata, const char* const* filelist, int filter)
{
	Chip8State* state = (Chip8State*)userdata;
	chip8_state_initialization(state);

	if (!filelist || !*filelist) {
		return;
	}

	const char* filePath = filelist[0];

	if (chip8_load_rom(state, filePath)) {
		SDL_Log("ROM Loaded successfully!");
	}
	else {
		SDL_Log("Failed to load ROM!");
	}
}

void ui_process_event(SDL_Event* event)
{
	ImGui_ImplSDL3_ProcessEvent(event);
}

void ui_initalization(float main_scale, SDL_Window* window, SDL_GPUDevice* device)
{
	igCreateContext(NULL);
	ImGuiIO* io = igGetIO(); (void)io;

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

	init_info.Device = device;
	init_info.ColorTargetFormat = SDL_GetGPUSwapchainTextureFormat(
		device,
		window);

	init_info.MSAASamples = SDL_GPU_SAMPLECOUNT_1;
	ImGui_ImplSDLGPU3_Init(&init_info);
}

ImDrawData* ui_renderer(Chip8State* state, SDL_Window* window)
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
						state,
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
	return igGetDrawData();
}

void ui_prepare_draw_data(ImDrawData* draw_data, SDL_GPUCommandBuffer* commandBuffer) {
	ImGui_ImplSDLGPU3_PrepareDrawData(draw_data, commandBuffer);
}

void ui_render_draw_data(
	ImDrawData* draw_data,
	SDL_GPUCommandBuffer* commandBuffer, 
	SDL_GPURenderPass* renderPass) 
{
	ImGui_ImplSDLGPU3_RenderDrawData(draw_data, commandBuffer, renderPass, NULL);
}