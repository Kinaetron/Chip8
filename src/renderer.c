#include "renderer.h"

SDL_GPUShader* LoadShader(
	SDL_GPUDevice* device, 
	const char* shaderFilename, 
	uint32_t samplerCount, 
	uint32_t uniformBufferCount, 
	uint32_t storageBufferCount,  
	uint32_t storageTextureCount)
{
	SDL_GPUShaderStage stage;
	if (SDL_strstr(shaderFilename, ".vert")) {
		stage = SDL_GPU_SHADERSTAGE_VERTEX;
	}
	else if (SDL_strstr(shaderFilename, ".frag")) {
		stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
	}
	else 
	{
		SDL_Log("Invalid shader stage!");
		return NULL;
	}

	char fullPath[256];
	SDL_GPUShaderFormat backendFormats = SDL_GetGPUShaderFormats(device);
	SDL_GPUShaderFormat format = SDL_GPU_SHADERFORMAT_INVALID;
	const char* entrypoint;

	const char* basePath = SDL_GetBasePath();

	if (backendFormats & SDL_GPU_SHADERFORMAT_SPIRV) 
	{
		SDL_snprintf(fullPath, sizeof(fullPath), "%sshaders/compiled/%s.spv", basePath, shaderFilename);
		format = SDL_GPU_SHADERFORMAT_SPIRV;
		entrypoint = "main";
	}
	else if (backendFormats & SDL_GPU_SHADERFORMAT_MSL)
	{
		SDL_snprintf(fullPath, sizeof(fullPath), "%sshaders/compiled/%s.msl", basePath, shaderFilename);
		format = SDL_GPU_SHADERFORMAT_MSL;
		entrypoint = "main0";
	}
	else if (backendFormats & SDL_GPU_SHADERFORMAT_DXIL)
	{
		SDL_snprintf(fullPath, sizeof(fullPath), "%sshaders/compiled/%s.dxil", basePath, shaderFilename);
		format = SDL_GPU_SHADERFORMAT_DXIL;
		entrypoint = "main";
	}
	else
	{
		SDL_Log("%s", "Unrecognized backend shader format!");
		return NULL;
	}

	size_t codeSize;
	void* code = SDL_LoadFile(fullPath, &codeSize);
	if (code == NULL)
	{
		SDL_Log("Failed to load compute shader from disk! %s", fullPath);
		return NULL;
	}

	SDL_GPUShaderCreateInfo shaderInfo =
	{
		.code = code,
		.code_size = codeSize,
		.entrypoint = entrypoint,
		.format = format,
		.stage = stage,
		.num_samplers = (Uint32)samplerCount,
		.num_uniform_buffers = (Uint32)uniformBufferCount,
		.num_storage_buffers = (Uint32)storageBufferCount,
		.num_storage_textures = (Uint32)storageTextureCount
	};

	SDL_GPUShader* shader = SDL_CreateGPUShader(device, &shaderInfo);
	if (shader == NULL)
	{
		SDL_Log("Failed to create shader!");
		SDL_free(code);
		return NULL;
	}

	SDL_free(code);
	return shader;
}

int InitializeRenderer(SDL_GPUDevice* device, SDL_Window* window, GraphicsContext* context)
{
	SDL_GPUShader* vertexShader = LoadShader(device, "TexturedQuad.vert", 0, 0, 0, 0);
	if (vertexShader == NULL)
	{
		SDL_Log("Failed to create vertex shader %s", SDL_GetError());
		return -1;
	}

	SDL_GPUShader* fragmentShader = LoadShader(device, "TexturedQuad.frag", 1, 0, 0, 0);
	if (fragmentShader == NULL)
	{
		SDL_Log("Failed to create fragment shader %s", SDL_GetError());
		return -1;
	}

	SDL_GPUTextureFormat swapchainFormat = SDL_GetGPUSwapchainTextureFormat(device, window);
	if (swapchainFormat == SDL_GPU_TEXTUREFORMAT_INVALID) 
	{
		SDL_Log("Swapchain format is invalid! Did you forget to claim the window?");
		return -1;
	}

	SDL_GPUGraphicsPipelineCreateInfo pipelineCreateInfo = {
		.target_info = {
			.num_color_targets = 1,
			.color_target_descriptions = (SDL_GPUColorTargetDescription[]){{
				.format = swapchainFormat
			}},
		},
		.vertex_input_state = (SDL_GPUVertexInputState){
			.num_vertex_buffers = 1,
			.vertex_buffer_descriptions = (SDL_GPUVertexBufferDescription[]){{
				.slot = 0,
				.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
				.instance_step_rate = 0,
				.pitch = sizeof(PositionTextureVertex)
			}},
			.num_vertex_attributes = 2,
			.vertex_attributes = (SDL_GPUVertexAttribute[]){{
				.buffer_slot = 0,
				.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
				.location = 0,
				.offset = 0
			}, {
				.buffer_slot = 0,
				.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
				.location = 1,
				.offset = sizeof(float) * 3
			}}
		},
		.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
		.vertex_shader = vertexShader,
		.fragment_shader = fragmentShader
	};


	context->pipeline = SDL_CreateGPUGraphicsPipeline(device, &pipelineCreateInfo);
	if (context->pipeline == NULL)
	{
		SDL_Log("Failed to create pipeline!");
		return -1;
	}

	SDL_ReleaseGPUShader(device, vertexShader);
	SDL_ReleaseGPUShader(device, fragmentShader);

	context->sampler = SDL_CreateGPUSampler(device, &(SDL_GPUSamplerCreateInfo)
	{
		.min_filter = SDL_GPU_FILTER_NEAREST,
		.mag_filter = SDL_GPU_FILTER_NEAREST,
		.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST,
		.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
		.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
		.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
	});

	context->vertexBuffer = SDL_CreateGPUBuffer(device, &(SDL_GPUBufferCreateInfo) 
	{
		.usage = SDL_GPU_BUFFERUSAGE_VERTEX,
		.size = sizeof(PositionTextureVertex) * 4
	});

	SDL_SetGPUBufferName(
		device,
		context->vertexBuffer,
		"Chip 8 Screen Buffer"
	);


	context->indexBuffer = SDL_CreateGPUBuffer(device, &(SDL_GPUBufferCreateInfo){
		.usage = SDL_GPU_BUFFERUSAGE_INDEX,
		.size = sizeof(uint16_t) * 6
	});
	
	context->screenTexture = SDL_CreateGPUTexture(device, &(SDL_GPUTextureCreateInfo){
		.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
		.type = SDL_GPU_TEXTURETYPE_2D,
		.width = 64,
		.height = 32,
		.layer_count_or_depth = 1,
		.num_levels = 1,
		.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER
	});

	SDL_SetGPUTextureName(
		device,
		context->screenTexture,
		"Screen Texture"
	);

	context->transferBuffer = SDL_CreateGPUTransferBuffer(
		device,
		&(SDL_GPUTransferBufferCreateInfo) {
		.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
			.size = (sizeof(PositionTextureVertex) * 4) + (sizeof(Uint16) * 6)
	});

	PositionTextureVertex* vertexData = SDL_MapGPUTransferBuffer(
		device, 
		context->transferBuffer, 
		false);

	vertexData[0] = (PositionTextureVertex){ -1.0f,  1.0f, 0.0f, 0.0f, 0.0f };
	vertexData[1] = (PositionTextureVertex){ 1.0f,  1.0f, 0.0f, 1.0f, 0.0f };
	vertexData[2] = (PositionTextureVertex){ 1.0f, -1.0f, 0.0f, 1.0f, 1.0f };
	vertexData[3] = (PositionTextureVertex){ -1.0f, -1.0f, 0.0f, 0.0f, 1.0f };

	uint16_t* indexData = (uint16_t*)&vertexData[4];
	indexData[0] = 0; 
	indexData[1] = 1; 
	indexData[2] = 2;
	indexData[3] = 0; 
	indexData[4] = 2; 
	indexData[5] = 3;

	SDL_UnmapGPUTransferBuffer(device, context->transferBuffer);

	return 0;
}
