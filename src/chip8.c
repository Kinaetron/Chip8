#include "chip8.h"

#include <stdio.h>

void chip8_state_initialization(Chip8State* state)
{
	if (state == NULL) {
		return;
	}

	memset(state, 0, sizeof(Chip8State));

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