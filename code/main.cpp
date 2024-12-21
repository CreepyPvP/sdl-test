#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <stddef.h>
#include <math.h>

#include <SDL3/SDL.h>

typedef int32_t i32;
typedef uint8_t u8;
typedef uint32_t u32;
typedef uint64_t u64;
typedef float f32;

#include "game_math.cpp"

i32 WindowWidth = 1280;
i32 WindowHeight = 720;

struct vertex
{
    v3 Position;
};

struct state
{
    SDL_GPUDevice *Device;
};

struct global_uniforms
{
    mat4 Projection;
    mat4 View;
};

state State = {};

SDL_GPUShader *LoadShader(const char *Path, u32 UniformBufferCount, bool Fragment)
{
    SDL_GPUShaderStage Stage = SDL_GPU_SHADERSTAGE_VERTEX;
    if (Fragment)
    {
        Stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
    }

    u64 ShaderSourceSize;
    void *ShaderSource = SDL_LoadFile(Path, &ShaderSourceSize);
    assert(ShaderSource);

    SDL_GPUShaderCreateInfo ShaderInfo = {};
    ShaderInfo.code_size = ShaderSourceSize;
    ShaderInfo.code = (u8 *) ShaderSource;
    ShaderInfo.entrypoint = "main";
    ShaderInfo.format = SDL_GPU_SHADERFORMAT_SPIRV;
    ShaderInfo.stage = Stage;
    ShaderInfo.num_uniform_buffers = UniformBufferCount;

    SDL_GPUShader *Shader = SDL_CreateGPUShader(State.Device, &ShaderInfo);
    assert(Shader);

    SDL_free(ShaderSource);

    return Shader;
}

i32 main()
{
    SDL_Init(SDL_INIT_VIDEO);

    SDL_SetLogPriorities(SDL_LOG_PRIORITY_DEBUG);

    SDL_Window *Window =  SDL_CreateWindow("My game", WindowWidth, WindowHeight, SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN);
    State.Device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, NULL);

    if (!SDL_ClaimWindowForGPUDevice(State.Device, Window))
    {
        printf("Failed to assign device to window\n");
        return 1;
    }

    SDL_GPUShader *FragmentShader = LoadShader("assets/default.frag.spv", 0, true);
    SDL_GPUShader *VertexShader = LoadShader("assets/default.vert.spv", 1, false);

    // Pipeline...
    //
    SDL_GPUVertexBufferDescription BufferDescription = {};
    BufferDescription.slot = 0;
    BufferDescription.pitch = sizeof(vertex);
    BufferDescription.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;

    SDL_GPUVertexAttribute AttributePosition = {};
    AttributePosition.buffer_slot = 0;
    AttributePosition.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
    AttributePosition.location = 0;
    AttributePosition.offset = offsetof(vertex, Position);

    SDL_GPUColorTargetDescription SwapchainTargetDescription = {};
    SwapchainTargetDescription.format = SDL_GetGPUSwapchainTextureFormat(State.Device, Window);

    SDL_GPUGraphicsPipelineCreateInfo PipelineInfo = {};
    PipelineInfo.vertex_shader = VertexShader;
    PipelineInfo.fragment_shader = FragmentShader;
    PipelineInfo.vertex_input_state.vertex_buffer_descriptions = &BufferDescription;
    PipelineInfo.vertex_input_state.num_vertex_buffers = 1;
    PipelineInfo.vertex_input_state.vertex_attributes = &AttributePosition;
    PipelineInfo.vertex_input_state.num_vertex_attributes = 1;
    PipelineInfo.target_info.color_target_descriptions = &SwapchainTargetDescription;
    PipelineInfo.target_info.num_color_targets = 1;

    SDL_GPUGraphicsPipeline *Pipeline =  SDL_CreateGPUGraphicsPipeline(State.Device, &PipelineInfo);
    assert(Pipeline);

    SDL_ReleaseGPUShader(State.Device, VertexShader);
	SDL_ReleaseGPUShader(State.Device, FragmentShader);

    // Vertex Buffer
    //
    SDL_GPUBufferCreateInfo VertexBufferInfo = {};
    VertexBufferInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    VertexBufferInfo.size = sizeof(vertex) * 3;
    SDL_GPUBuffer *VertexBuffer = SDL_CreateGPUBuffer(State.Device, &VertexBufferInfo);

    SDL_GPUTransferBufferCreateInfo TransferBufferInfo = {};
    TransferBufferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    TransferBufferInfo.size = sizeof(vertex) * 3;
    SDL_GPUTransferBuffer *TransferBuffer = SDL_CreateGPUTransferBuffer(State.Device, &TransferBufferInfo);

    vertex *TransferData = (vertex *) SDL_MapGPUTransferBuffer(State.Device, TransferBuffer, false);
    TransferData[0].Position = V3(-0.3, -0.3, 0.0);
    TransferData[1].Position = V3(0.3, -0.3, 0.0);
    TransferData[2].Position = V3(0.0, 0.3, 0.0);
    // TransferData[0].Position = V3(-0.3, 0, -0.3);
    // TransferData[1].Position = V3(0.3, 0, -0.3);
    // TransferData[2].Position = V3(0.0, 0, 0.3);
    SDL_UnmapGPUTransferBuffer(State.Device, TransferBuffer);

    SDL_GPUCommandBuffer *UploadCommandBuffer = SDL_AcquireGPUCommandBuffer(State.Device);
    SDL_GPUCopyPass *CopyPass = SDL_BeginGPUCopyPass(UploadCommandBuffer);

    SDL_GPUTransferBufferLocation Source = {};
    Source.transfer_buffer = TransferBuffer;

    SDL_GPUBufferRegion Dest = {};
    Dest.buffer = VertexBuffer;
    Dest.size = sizeof(vertex) * 3;

	SDL_UploadToGPUBuffer(CopyPass, &Source, &Dest, false);

	SDL_EndGPUCopyPass(CopyPass);
	SDL_SubmitGPUCommandBuffer(UploadCommandBuffer);
	SDL_ReleaseGPUTransferBuffer(State.Device, TransferBuffer);

    // Main loop...
    //
    bool WindowIsOpen = true;

    SDL_Event Event;
    while (WindowIsOpen)
    {
        while (SDL_PollEvent(&Event))
        {
            switch (Event.type)
            {
                case SDL_EVENT_KEY_DOWN: {
                    if (Event.key.key == SDLK_ESCAPE)
                    {
                        WindowIsOpen = false;
                    }
                    break;
                }
                case SDL_EVENT_QUIT: {
                    WindowIsOpen = false;
                    break;
                }
            }
        }

        SDL_GPUCommandBuffer *CommandBuffer = SDL_AcquireGPUCommandBuffer(State.Device);

        SDL_GPUTexture *SwapchainTexture;
        SDL_AcquireGPUSwapchainTexture(CommandBuffer, Window, &SwapchainTexture, NULL, NULL);

        global_uniforms GlobalUniforms = {};
        GlobalUniforms.Projection = Perspective(Radians(50), (f32) WindowWidth / (f32) WindowHeight, 0.01, 1000);
        GlobalUniforms.View = LookAt(V3(0, 0, 1), V3(0), V3(0, 1, 0));

        // NOTE: When window is minimized there is no swapchain image, so SwapchainTexture will be NULL
        if (SwapchainTexture)
        {
            SDL_GPUColorTargetInfo ColorTargetInfo = {};
            ColorTargetInfo.texture = SwapchainTexture;
            ColorTargetInfo.clear_color = { 0.1f, 0.1f, 0.2f, 1.0f };
            ColorTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
            ColorTargetInfo.store_op = SDL_GPU_STOREOP_STORE;

            SDL_GPURenderPass *RenderPass = SDL_BeginGPURenderPass(CommandBuffer, &ColorTargetInfo, 1, NULL);
            SDL_BindGPUGraphicsPipeline(RenderPass, Pipeline);

            SDL_GPUBufferBinding VertexBufferBinding = {};
            VertexBufferBinding.buffer = VertexBuffer;
            SDL_BindGPUVertexBuffers(RenderPass, 0, &VertexBufferBinding, 1);

            SDL_PushGPUVertexUniformData(CommandBuffer, 0, &GlobalUniforms, sizeof(GlobalUniforms));

            SDL_DrawGPUPrimitives(RenderPass, 3, 1, 0, 0);
            SDL_EndGPURenderPass(RenderPass);
        }

        SDL_SubmitGPUCommandBuffer(CommandBuffer);
    }
}
