#ifndef SHADER_H
#define SHADER_H

#include <stdint.h>
#include <SDL3/SDL.h>


SDL_GPUShader* LoadShader(
	SDL_GPUDevice* device,
	const char* shaderFilename,
	uint32_t samplerCount,
	uint32_t uniformBufferCount,
	uint32_t storageTextureCount
);

#endif