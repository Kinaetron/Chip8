#include "chip8.h"

#include <stdio.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_keycode.h>

#define FONT_SIZE 80

uint8_t chip8_fontset[FONT_SIZE] =
{
	0xF0, 0x90, 0x90, 0x90, 0xF0,
	0x20, 0x60, 0x20, 0x20, 0x70,
	0xF0, 0x10, 0xF0, 0x80, 0xF0,
	0xF0, 0x10, 0xF0, 0x10, 0xF0,
	0x90, 0x90, 0xF0, 0x10, 0x10,
	0xF0, 0x80, 0xF0, 0x10, 0xF0,
	0xF0, 0x80, 0xF0, 0x90, 0xF0,
	0xF0, 0x10, 0x20, 0x40, 0x40,
	0xF0, 0x90, 0xF0, 0x90, 0xF0,
	0xF0, 0x90, 0xF0, 0x10, 0xF0,
	0xF0, 0x90, 0xF0, 0x90, 0x90,
	0xE0, 0x90, 0xE0, 0x90, 0xE0,
	0xF0, 0x80, 0x80, 0x80, 0xF0,
	0xE0, 0x90, 0x90, 0x90, 0xE0,
	0xF0, 0x80, 0xF0, 0x80, 0xF0,
	0xF0, 0x80, 0xF0, 0x80, 0x80
};

void chip8_state_initialization(Chip8State* state)
{
	if (state == NULL) {
		return;
	}

	memset(state, 0, sizeof(Chip8State));
	memcpy(state->memory, chip8_fontset, FONT_SIZE * sizeof(uint8_t));

	state->program_counter = CHIP8_START_ADDRESS;
}

bool chip8_load_rom(Chip8State* state, const char* filePath)
{
	FILE* file = fopen(filePath, "rb");

	if (!file) {
		return false;
	}

	fseek(file, 0, SEEK_END);
	long fileSize = ftell(file);
	rewind(file);

	long max_rom_size = sizeof(state->memory) - CHIP8_START_ADDRESS;
	if (fileSize > max_rom_size) {
		fclose(file);
		return false;
	}

	size_t bytesRead = fread(&state->memory[CHIP8_START_ADDRESS], 1, fileSize, file);
	fclose(file);

	return (bytesRead == (size_t)fileSize);
}

void chip_input_state(SDL_Event* event, bool* inputState)
{
	bool isPressed = (event->type == SDL_EVENT_KEY_DOWN);
	switch (event->key.key)
	{
		case SDLK_1:
			inputState[0] = isPressed;
			break;

		case SDLK_2:
			inputState[1] = isPressed;
			break;

		case SDLK_3:
			inputState[2] = isPressed;
			break;

		case SDLK_4:
			inputState[3] = isPressed;
			break;

		case SDLK_Q:
			inputState[4] = isPressed;
			break;

		case SDLK_W:
			inputState[5] = isPressed;
			break;

		case SDLK_E:
			inputState[6] = isPressed;
			break;

		case SDLK_R:
			inputState[7] = isPressed;
			break;

		case SDLK_A:
			inputState[8] = isPressed;
			break;

		case SDLK_S:
			inputState[9] = isPressed;
			break;

		case SDLK_D:
			inputState[10] = isPressed;
			break;

		case SDLK_F:
			inputState[11] = isPressed;
			break;

		case SDLK_Z:
			inputState[12] = isPressed;
			break;

		case SDLK_X:
			inputState[13] = isPressed;
			break;

		case SDLK_C:
			inputState[14] = isPressed;
			break;

		case SDLK_V:
			inputState[15] = isPressed;
			break;
	}
}