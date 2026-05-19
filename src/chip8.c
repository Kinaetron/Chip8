#include "chip8.h"

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_keycode.h>
#include <stdint.h>

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

	srand((unsigned int)time(NULL));
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

static void op_0x00EE(Chip8State* state)
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

static void op_0x6XNN(Chip8State* state, uint8_t Vx)
{
	uint8_t value = state->opcode & 0x00FF;

	state->registers[Vx] = value;
}

static void op_0x7XNN(Chip8State* state, uint8_t Vx)
{
	uint8_t value = state->opcode & 0x00FF;

	state->registers[Vx] += value;
}

static void op_0xANNN(Chip8State* state)
{
	uint16_t address = state->opcode & 0x0FFF;
	state->index = address;
}

static void op_0x3XNN(Chip8State* state, uint8_t Vx)
{
	uint8_t byte = state->opcode & 0x00FF;

	if (state->registers[Vx] == byte) {
		state->program_counter += 2;
	}
}

static void op_0x4XNN(Chip8State* state, uint8_t Vx)
{
	uint8_t byte = state->opcode & 0x00FF;

	if (state->registers[Vx] != byte) {
		state->program_counter += 2;
	}
}

static void op_0x5XY0(Chip8State* state, uint8_t Vx, uint8_t Vy)
{
	if (state->registers[Vx] == state->registers[Vy]) {
		state->program_counter += 2;
	}
}

static void op_0x8XY0(Chip8State* state, uint8_t Vx, uint8_t Vy)
{
	state->registers[Vx] = state->registers[Vy];
}

static void op_0x8XY1(Chip8State* state, uint8_t Vx, uint8_t Vy)
{
	state->registers[Vx] |= state->registers[Vy];
}

static void op_0x8XY2(Chip8State* state, uint8_t Vx, uint8_t Vy)
{
	state->registers[Vx] &= state->registers[Vy];
}

static void op_0x8XY3(Chip8State* state, uint8_t Vx, uint8_t Vy)
{
	state->registers[Vx] ^= state->registers[Vy];
}

static void op_0x8XY4(Chip8State* state, uint8_t Vx, uint8_t Vy)
{
	uint16_t sum = state->registers[Vx] + state->registers[Vy];

	if (sum > 255) {
		state->registers[0xF] = 1;
	}
	else {
		state->registers[0xF] = 0;
	}

	state->registers[Vx] = sum & 0xFF;
}

static void op_0x8XY5(Chip8State* state, uint8_t Vx, uint8_t Vy)
{
	if (state->registers[Vx] > state->registers[Vy]) {
		state->registers[0xF] = 1;
	}
	else {
		state->registers[0xF] = 0;
	}

	state->registers[Vx] -= state->registers[Vy];
}

static void op_0x8XY6(Chip8State* state, uint8_t Vx)
{
	state->registers[0xF] = (state->registers[Vx] & 0x1);
	state->registers[Vx] >>= 1;
}

static void op_0x8XY7(Chip8State* state, uint8_t Vx, uint8_t Vy)
{
	if (state->registers[Vy] > state->registers[Vx]) {
		state->registers[0xF] = 1;
	}
	else {
		state->registers[0xF] = 0;
	}

	state->registers[Vx] = state->registers[Vy] - state->registers[Vx];
}

static void op_0x8XYE(Chip8State* state, uint8_t Vx)
{
	state->registers[0xF] = (state->registers[Vx] & 0x80) >> 7;
	state->registers[Vx] <<= 1;
}

static void op_0x9XY0(Chip8State* state, uint8_t Vx, uint8_t Vy)
{
	if (state->registers[Vx] != state->registers[Vy]) {
		state->program_counter += 2;
	}
}

static void op_0xBNNN(Chip8State* state)
{
	uint16_t address = state->opcode & 0x0FFF;
	state->program_counter = state->registers[0] + address;
}

static void op_0xCXNN(Chip8State* state, uint8_t Vx)
{
	uint8_t byte = state->opcode & 0x00FF;

	state->registers[Vx] = (rand() % 256) & byte;
}

static void op_0xDXYN(Chip8State* state, uint8_t Vx, uint8_t Vy)
{
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

static void op_0xEX9E(Chip8State* state, uint8_t Vx)
{
	uint8_t key = state->registers[Vx];

	if (state->keypad[key]) {
		state->program_counter += 2;
	}
}

static void op_0xEXA1(Chip8State* state, uint8_t Vx)
{
	uint8_t key = state->registers[Vx];

	if (!state->keypad[key]) {
		state->program_counter += 2;
	}
}

static void op_0xFX07(Chip8State* state, uint8_t Vx)
{
	state->registers[Vx] = state->delay_timer;
}

static void op_0xFX0A(Chip8State* state, uint8_t Vx)
{
	for (int i = 0; i < 16; i++)
	{
		if (state->keypad[i]) 
		{
			state->registers[Vx] = i;
			return;
		}
	}

	state->program_counter -= 2;
}

static void op_0xFX15(Chip8State* state, uint8_t Vx) {
	state->delay_timer = state->registers[Vx];
}

static void op_OxFX18(Chip8State* state, uint8_t Vx) {
	state->sound_timer = state->registers[Vx];
}

static void op_OxFX1E(Chip8State* state, uint8_t Vx) {
	state->index += state->registers[Vx];
}

static void op_0xFX29(Chip8State* state, uint8_t Vx)
{
	uint8_t digit = state->registers[Vx];

	state->index = (5 * digit);
}

static void op_0xFX33(Chip8State* state, uint8_t Vx)
{
	uint8_t value = state->registers[Vx];

	state->memory[state->index + 2] = value % 10;
	value /= 10;

	state->memory[state->index + 1] = value % 10;
	value /= 10;

	state->memory[state->index] = value % 10;
}

static void op_0xFX55(Chip8State* state, uint8_t Vx)
{
	for (uint8_t i = 0; i <= Vx; i++) {
		state->memory[state->index + i] = state->registers[i];
	}
}

static void op_0xFX65(Chip8State* state, uint8_t Vx)
{
	for (uint8_t i = 0; i <= Vx; i++) {
		state->memory[i] = state->registers[i];
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

	uint8_t Vx = (state->opcode & 0x0F00) >> 8;
	uint8_t Vy = (state->opcode & 0x00F0) >> 4;

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
					op_0x00EE(state);

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
			op_0x3XNN(state, Vx);
			break;

		case 0x4000:
			op_0x4XNN(state, Vx);
			break;

		case 0x5000:
			op_0x5XY0(state, Vx, Vy);
			break;

		case 0x6000:
			op_0x6XNN(state, Vx);
			break;

		case 0x7000:
			op_0x7XNN(state, Vx);
			break;

		case 0x8000:
		{
			switch (state->opcode & 0x000F)
			{
			case 0x0:
				op_0x8XY0(state, Vx, Vy);
				break;

			case 0x1:
				op_0x8XY1(state, Vx, Vy);
				break;

			case 0x2:
				op_0x8XY2(state, Vx, Vy);
				break;

			case 0x3:
				op_0x8XY3(state, Vx, Vy);
				break;

			case 0x4:
				op_0x8XY4(state, Vx, Vy);
				break;

			case 0x5:
				op_0x8XY5(state, Vx, Vy);
				break;

			case 0x6:
				op_0x8XY6(state, Vx);
				break;

			case 0x7:
				op_0x8XY7(state, Vx, Vy);
				break;

			case 0xE:
				op_0x8XYE(state, Vx);
				break;
			}
		} break;

		case 0x9000:
			op_0x9XY0(state, Vx, Vy);
			break;

		case 0xA000:
			op_0xANNN(state);
			break;

		case 0xB000:
			op_0xBNNN(state);
			break;

		case 0xC000:
			op_0xCXNN(state, Vx);
			break;

		case 0xD000:
			op_0xDXYN(state, Vx, Vy);
			break;

		case 0xE000:
		{
			switch (state->opcode & 0x00FF)
			{
				case 0x9E:
					op_0xEX9E(state, Vx);
					break;

				case 0xA1:
					op_0xEXA1(state, Vx);
					break;
			}

		}break;

		case 0xF000:
		{
			switch (state->opcode & 0x00FF)
			{
				case 0x07: 
					op_0xFX07(state, Vx);
					break;

				case 0x0A:
					op_0xFX0A(state, Vx);
					break;

				case 0x15:
					op_0xFX15(state, Vx);
					break;

				case 0x18:
					op_OxFX18(state, Vx);
					break;

				case 0x1E:
					op_OxFX1E(state, Vx);
					break;

				case 0x29:
					op_0xFX29(state, Vx);
					break;

				case 0x33:
					op_0xFX33(state, Vx);
					break;

				case 0x55:
					op_0xFX55(state, Vx);
					break;

				case 0x65:
					op_0xFX65(state, Vx);
					break;
			}
		} break;
	}
}