#include "ui.h"
#include "context.h"
#include "renderer.h"

SDL_GPUShader* load_shader(
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

SDL_AppResult initialize_renderer(Context* context)
{
	SDL_SetAppMetadata("Chip 8 Emulator", "1.0", "com.chip8-emulator");
	if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD | SDL_INIT_AUDIO))
	{
		SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	float main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
	SDL_WindowFlags window_flags = SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY;

	context->window = SDL_CreateWindow("Chip 8 Emulator", (int)(640 * main_scale), (int)(320 * main_scale) + 15, window_flags);
	if (context->window == NULL)
	{
		SDL_Log("Couldn't create window: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	SDL_SetWindowPosition(context->window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
	SDL_ShowWindow(context->window);

	context->gpu_device = SDL_CreateGPUDevice(
		SDL_GPU_SHADERFORMAT_SPIRV |
		SDL_GPU_SHADERFORMAT_DXIL |
		SDL_GPU_SHADERFORMAT_MSL |
		SDL_GPU_SHADERFORMAT_METALLIB,
		true,
		NULL);

	if (context->gpu_device == NULL)
	{
		SDL_Log("Error: SDL_CreateGPUDevice: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	if (!SDL_ClaimWindowForGPUDevice(context->gpu_device, context->window))
	{
		SDL_Log("Error: SDL_CreateGPUDevice Window Claim: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	SDL_SetGPUSwapchainParameters(
		context->gpu_device,
		context->window,
		SDL_GPU_SWAPCHAINCOMPOSITION_SDR,
		SDL_GPU_PRESENTMODE_VSYNC);

	ui_initalization(main_scale, context->window, context->gpu_device);

	SDL_GPUShader* vertexShader = load_shader(context->gpu_device, "chip8.vert", 0, 0, 0, 0);
	if (vertexShader == NULL)
	{
		SDL_Log("Failed to create vertex shader %s", SDL_GetError());
		return -1;
	}

	SDL_GPUShader* fragmentShader = load_shader(context->gpu_device, "chip8.frag", 1, 0, 0, 0);
	if (fragmentShader == NULL)
	{
		SDL_Log("Failed to create fragment shader %s", SDL_GetError());
		return -1;
	}

	SDL_GPUTextureFormat swapchainFormat = SDL_GetGPUSwapchainTextureFormat(context->gpu_device, context->window);
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


	context->graphicsContext->pipeline = SDL_CreateGPUGraphicsPipeline(context->gpu_device, &pipelineCreateInfo);
	if (context->graphicsContext->pipeline == NULL)
	{
		SDL_Log("Failed to create pipeline!");
		return -1;
	}

	SDL_ReleaseGPUShader(context->gpu_device, vertexShader);
	SDL_ReleaseGPUShader(context->gpu_device, fragmentShader);

	context->graphicsContext->sampler = SDL_CreateGPUSampler(context->gpu_device, &(SDL_GPUSamplerCreateInfo)
	{
		.min_filter = SDL_GPU_FILTER_NEAREST,
		.mag_filter = SDL_GPU_FILTER_NEAREST,
		.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST,
		.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
		.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
		.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
	});

	context->graphicsContext->vertexBuffer = SDL_CreateGPUBuffer(context->gpu_device, &(SDL_GPUBufferCreateInfo)
	{
		.usage = SDL_GPU_BUFFERUSAGE_VERTEX,
		.size = sizeof(PositionTextureVertex) * 4
	});

	SDL_SetGPUBufferName(
		context->gpu_device,
		context->graphicsContext->vertexBuffer,
		"Chip 8 Screen Buffer"
	);

	context->graphicsContext->indexBuffer = SDL_CreateGPUBuffer(context->gpu_device, &(SDL_GPUBufferCreateInfo){
		.usage = SDL_GPU_BUFFERUSAGE_INDEX,
		.size = sizeof(uint16_t) * 6
	});
	
	context->graphicsContext->screenTexture = SDL_CreateGPUTexture(context->gpu_device, &(SDL_GPUTextureCreateInfo){
		.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
		.type = SDL_GPU_TEXTURETYPE_2D,
		.width = CHIP8_SCREEN_WIDTH,
		.height = CHIP8_SCREEN_HEIGHT,
		.layer_count_or_depth = 1,
		.num_levels = 1,
		.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER
	});

	SDL_SetGPUTextureName(
		context->gpu_device,
		context->graphicsContext->screenTexture,
		"Screen Texture"
	);

	SDL_GPUTransferBuffer* bufferTransferBuffer = SDL_CreateGPUTransferBuffer(
		context->gpu_device,
		&(SDL_GPUTransferBufferCreateInfo) {
		.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
			.size = (sizeof(PositionTextureVertex) * 4) + (sizeof(Uint16) * 6)
	});

	PositionTextureVertex* transferData = SDL_MapGPUTransferBuffer(
		context->gpu_device,
		bufferTransferBuffer,
		false
	);
	
	transferData[0] = (PositionTextureVertex){ -1.0f,  1.0f, 0.0f, 0.0f, 0.0f };
	transferData[1] = (PositionTextureVertex){ 1.0f,  1.0f, 0.0f, 1.0f, 0.0f };
	transferData[2] = (PositionTextureVertex){ 1.0f, -1.0f, 0.0f, 1.0f, 1.0f };
	transferData[3] = (PositionTextureVertex){ -1.0f, -1.0f, 0.0f, 0.0f, 1.0f };

	uint16_t* indexData = (uint16_t*)&transferData[4];
	indexData[0] = 0; 
	indexData[1] = 1; 
	indexData[2] = 2;
	indexData[3] = 0; 
	indexData[4] = 2; 
	indexData[5] = 3;

	SDL_UnmapGPUTransferBuffer(context->gpu_device, bufferTransferBuffer);

	SDL_GPUCommandBuffer* uploadCommandBuffer = SDL_AcquireGPUCommandBuffer(context->gpu_device);
	SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(uploadCommandBuffer);

	SDL_UploadToGPUBuffer(
		copyPass,
		&(SDL_GPUTransferBufferLocation) {
		.transfer_buffer = bufferTransferBuffer,
			.offset = 0
	},
		& (SDL_GPUBufferRegion) {
		.buffer = context->graphicsContext->vertexBuffer,
			.offset = 0,
			.size = sizeof(PositionTextureVertex) * 4
	},
		false
	);

	SDL_UploadToGPUBuffer(
		copyPass,
		&(SDL_GPUTransferBufferLocation) {
		.transfer_buffer = bufferTransferBuffer,
			.offset = sizeof(PositionTextureVertex) * 4
	},
		& (SDL_GPUBufferRegion) {
		.buffer = context->graphicsContext->indexBuffer,
			.offset = 0,
			.size = sizeof(Uint16) * 6
	},
		false
	);

	SDL_EndGPUCopyPass(copyPass);
	SDL_SubmitGPUCommandBuffer(uploadCommandBuffer);
	SDL_ReleaseGPUTransferBuffer(context->gpu_device, bufferTransferBuffer);

	context->graphicsContext->transferBuffer = SDL_CreateGPUTransferBuffer(
		context->gpu_device,
		&(SDL_GPUTransferBufferCreateInfo){
		.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
			.size = CHIP8_SCREEN_WIDTH * CHIP8_SCREEN_HEIGHT * sizeof(uint32_t)
	});
	return 0;
}

SDL_AppResult render(SDL_GPUDevice* device, GraphicsContext* context, SDL_Window* window, Chip8State* state)
{
	SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(device);

	if (commandBuffer == NULL)
	{
		SDL_Log("AcquireGPUCommandBuffer failed: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	ImDrawData* draw_data = ui_renderer(state, window);

	uint32_t* pixels = SDL_MapGPUTransferBuffer(device, context->transferBuffer, false);
	SDL_memcpy(pixels, state->video, CHIP8_SCREEN_WIDTH * CHIP8_SCREEN_HEIGHT * sizeof(uint32_t));
	SDL_UnmapGPUTransferBuffer(device, context->transferBuffer);

	SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(commandBuffer);


	SDL_UploadToGPUTexture(
		copyPass,
		&(SDL_GPUTextureTransferInfo) {
		.transfer_buffer = context->transferBuffer,
			.offset = 0,
	},
		& (SDL_GPUTextureRegion) {
		.texture = context->screenTexture,
			.w = CHIP8_SCREEN_WIDTH,
			.h = CHIP8_SCREEN_HEIGHT,
			.d = 1
	},
		false
	);

	SDL_EndGPUCopyPass(copyPass);

	SDL_GPUTexture* swapchainTexture;

	if (!SDL_WaitAndAcquireGPUSwapchainTexture(
		commandBuffer,
		window,
		&swapchainTexture,
		NULL,
		NULL))
	{
		SDL_Log("WaitAndAcquireGPUSwapchainTexture failed: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	if (swapchainTexture != NULL)
	{
		ui_prepare_draw_data(draw_data, commandBuffer);

		SDL_GPUColorTargetInfo target_info = { 0 };
		target_info.texture = swapchainTexture;
		target_info.load_op = SDL_GPU_LOADOP_CLEAR;
		target_info.store_op = SDL_GPU_STOREOP_STORE;
		target_info.clear_color = (SDL_FColor){ 0.0f, 0.0f, 0.0f, 1.0f };
		target_info.load_op = SDL_GPU_LOADOP_CLEAR;
		target_info.store_op = SDL_GPU_STOREOP_STORE;

		SDL_GPURenderPass* renderPass =
			SDL_BeginGPURenderPass(commandBuffer, &target_info, 1, NULL);

		SDL_BindGPUGraphicsPipeline(renderPass, context->pipeline);
		SDL_SetGPUViewport(renderPass, &(SDL_GPUViewport){ 0, 15, 640, 320 });
		SDL_BindGPUVertexBuffers(renderPass, 0, &(SDL_GPUBufferBinding){.buffer = context->vertexBuffer, .offset = 0 }, 1);
		SDL_BindGPUIndexBuffer(renderPass, &(SDL_GPUBufferBinding){.buffer = context->indexBuffer, .offset = 0 }, SDL_GPU_INDEXELEMENTSIZE_16BIT);
		SDL_BindGPUFragmentSamplers(renderPass, 0, &(SDL_GPUTextureSamplerBinding){.texture = context->screenTexture, .sampler = context->sampler }, 1);
		SDL_DrawGPUIndexedPrimitives(renderPass, 6, 1, 0, 0, 0);

		ui_render_draw_data(draw_data, commandBuffer, renderPass);

		SDL_EndGPURenderPass(renderPass);
	}

	SDL_SubmitGPUCommandBuffer(commandBuffer);

	return SDL_APP_CONTINUE;
}

void destroy_renderer(SDL_GPUDevice* device, GraphicsContext* context)
{
	SDL_ReleaseGPUGraphicsPipeline(device, context->pipeline);
	SDL_ReleaseGPUBuffer(device, context->vertexBuffer);
	SDL_ReleaseGPUBuffer(device, context->indexBuffer);
	SDL_ReleaseGPUTexture(device, context->screenTexture);
	SDL_ReleaseGPUTransferBuffer(device, context->transferBuffer);
	SDL_ReleaseGPUSampler(device, context->sampler);
}