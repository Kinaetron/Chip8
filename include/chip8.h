#ifndef CHIP8_H
#define CHIP8_H

#include <stdint.h>
#include <stdbool.h>
#include <SDL3/SDL_events.h>

#define CHIP8_MEMORY_SIZE 4096
#define CHIP8_START_ADDRESS 0x200

typedef struct
{
	uint8_t registers[16];
	uint8_t memory[CHIP8_MEMORY_SIZE];
	uint16_t index;
	uint16_t program_counter;
	uint16_t stack[16];
	uint8_t stack_pointer;
	uint8_t delay_timer;
	uint8_t sound_timer;
	bool keypad[16];
	uint32_t video[64 * 32];
	uint16_t opcode;
	bool rom_loaded;
} Chip8State;

void chip_input_state(SDL_Event* event, bool* inputState);
void chip8_state_initialization(Chip8State* state);
bool chip8_load_rom(Chip8State* state, const char* filePath);
void chip8_cycle(Chip8State* state);

#endif