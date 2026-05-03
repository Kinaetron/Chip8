#ifndef RENDERER_H
#define RENDERER_H

#include <stdint.h>
#include <SDL3/SDL.h>

typedef struct PositionTextureVertex
{
	float x, y, z;
	float u, v;
} PositionTextureVertex;

typedef struct
{
	SDL_GPUGraphicsPipeline* pipeline;
	SDL_GPUBuffer* vertexBuffer;
	SDL_GPUBuffer* indexBuffer;
	SDL_GPUTexture* screenTexture;
	SDL_GPUTransferBuffer* transferBuffer;
	SDL_GPUSampler* sampler;
} GraphicsContext;


SDL_GPUShader* LoadShader(
	SDL_GPUDevice* device,
	const char* shaderFilename,
	uint32_t samplerCount,
	uint32_t uniformBufferCount,
	uint32_t storageBufferCount,
	uint32_t storageTextureCount
);

int InitializeRenderer(SDL_GPUDevice* device, SDL_Window* window, GraphicsContext* context);

#endif