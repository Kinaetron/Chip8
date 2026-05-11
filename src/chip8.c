#include "chip8.h"

#include <stdio.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_keycode.h>

#define FONT_SIZE 80
#define PIXEL_ON  (Pixel){255,255,255,255}
#define PIXEL_OFF (Pixel){0,0,0,255}
#define SCREEN_SIZE 64 * 32

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
	chip8_state_initialization(state);
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

	state->rom_loaded = (bytesRead == (size_t)fileSize);

	return state->rom_loaded;
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

static void op_0x00E0(Chip8State* state) 
{
	for (int i = 0; i < SCREEN_SIZE; i++) {
		state->video[i] = PIXEL_OFF;
	}
}

static void op_Ox00EE(Chip8State* state)
{
	state->program_counter = 
		state->stack[--state->stack_pointer];
}

static void op_0x1NNN(Chip8State* state)
{
	uint16_t address = state->opcode & 0x0FFF;
	state->program_counter = address;
}

static void op_0x2NNN(Chip8State* state)
{
	uint16_t address = state->opcode & 0x0FFF;

	state->stack[state->stack_pointer++] = state->program_counter;
	state->program_counter = address;
}

static void op_0x6XNN(Chip8State* state)
{
	uint8_t vx = (state->opcode & 0x0F00) >> 8;
	uint8_t value = state->opcode & 0x00FF;

	state->registers[vx] = value;
}

static void op_0x7XNN(Chip8State* state)
{
	uint8_t vx = (state->opcode & 0x0F00) >> 8;
	uint8_t value = state->opcode & 0x00FF;

	state->registers[vx] += value;
}

static void op_OxANNN(Chip8State* state)
{
	uint16_t address = state->opcode & 0x0FFF;
	state->index = address;
}

static void op_Ox3XKK(Chip8State* state)
{
	uint8_t Vx = (state->opcode & 0xF00) >> 8;
	uint8_t byte = state->opcode & 0x00FF;

	if (state->registers[Vx] == byte) {
		state->program_counter += 2;
	}
}

static void op_Ox4XKK(Chip8State* state)
{
	uint8_t Vx = (state->opcode & 0xF00) >> 8;
	uint8_t byte = state->opcode & 0x00FF;

	if (state->registers[Vx] != byte) {
		state->program_counter += 2;
	}
}

static void op_OxDXYN(Chip8State* state)
{
	uint8_t Vx = (state->opcode & 0x0F00) >> 8;
	uint8_t Vy = (state->opcode & 0x00F0) >> 4;
	uint8_t height = state->opcode & 0x000F;

	uint8_t xPosition = state->registers[Vx] % SCREEN_WIDTH;
	uint8_t yPosition = state->registers[Vy] % SCREEN_HEIGHT;

	state->registers[0xF] = 0;

	for (unsigned int row = 0; row < height; row++)
	{
		uint8_t spriteByte = state->memory[state->index + row];

		for (unsigned int column = 0; column < 8; column++)
		{
			uint8_t spritePixel = spriteByte & (0x80 >> column);

			if (spritePixel)
			{
				uint32_t x = (xPosition + column) % SCREEN_WIDTH;
				uint32_t y = (yPosition + row) % SCREEN_HEIGHT;

				Pixel* screenPixel = &state->video[y * SCREEN_WIDTH + x];

				bool pixelWasOn = (screenPixel->r == 255);

				if (pixelWasOn)
				{
					state->registers[0xF] = 1;
					*screenPixel = PIXEL_OFF;
				}
				else {
					*screenPixel = PIXEL_ON;
				}
			}
		}
	}
}

void chip8_cycle(Chip8State* state)
{
	if (!state->rom_loaded) {
		return;
	}

	state->opcode =
		(state->memory[state->program_counter] << 8u) |
		 state->memory[state->program_counter + 1];

	state->program_counter += 2;

	switch (state->opcode & 0xF000)
	{
		case 0x0000:
		{
			switch (state->opcode & 0x00FF)
			{
				case 0x00E0:
					op_0x00E0(state);
					break;

				case 0x00EE:
					op_Ox00EE(state);

				default:
					printf("Unknown opcode: %04X\n", state->opcode);
					break;
			}
		} break;

		case 0x1000:
			op_0x1NNN(state);
			break;

		case 0x2000: 
			op_0x2NNN(state); 
			break;

		case 0x3000:
			op_Ox3XKK(state);
			break;

		case 0x4000:
			op_Ox4XKK(state);
			break;

		case 0x6000:
			op_0x6XNN(state);
			break;

		case 0x7000:
			op_0x7XNN(state);
			break;

		case 0xA000:
			op_OxANNN(state);
			break;

		case 0xD000:
			op_OxDXYN(state);
			break;
	}
}