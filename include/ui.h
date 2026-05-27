#ifndef UI_H
#define UI_H

#include <SDL3/SDL.h>

#include <cimgui.h>
#include <cimgui_impl.h>

#include "chip8.h"
#include <stdint.h>

extern uint32_t CHIP8_LOAD_ROM_EVENT;

#define igGetIO igGetIO_Nil

void ui_process_event(SDL_Event* event);
void ui_initalization(float main_scale, SDL_Window* window, SDL_GPUDevice* device);
ImDrawData* ui_renderer(Chip8State* state, SDL_Window* window);
void ui_prepare_draw_data(ImDrawData* draw_data, SDL_GPUCommandBuffer* commandBuffer);
void ui_render_draw_data(ImDrawData* draw_data, SDL_GPUCommandBuffer* commandBuffer,SDL_GPURenderPass* renderPass);

#endif