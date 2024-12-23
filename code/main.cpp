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
    v3 Normal;
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

void CopyToBuffer(SDL_GPUBuffer *Buffer, void *Data, u32 Bytes)
{
    SDL_GPUTransferBufferCreateInfo TransferBufferInfo = {};
    TransferBufferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    TransferBufferInfo.size = Bytes;
    SDL_GPUTransferBuffer *TransferBuffer = SDL_CreateGPUTransferBuffer(State.Device, &TransferBufferInfo);

    vertex *TransferData = (vertex *) SDL_MapGPUTransferBuffer(State.Device, TransferBuffer, false);
    SDL_memcpy(TransferData, Data, Bytes);
    SDL_UnmapGPUTransferBuffer(State.Device, TransferBuffer);

    SDL_GPUCommandBuffer *UploadCommandBuffer = SDL_AcquireGPUCommandBuffer(State.Device);
    SDL_GPUCopyPass *CopyPass = SDL_BeginGPUCopyPass(UploadCommandBuffer);

    SDL_GPUTransferBufferLocation Source = {};
    Source.transfer_buffer = TransferBuffer;

    SDL_GPUBufferRegion Dest = {};
    Dest.buffer = Buffer;
    Dest.size = Bytes;

	SDL_UploadToGPUBuffer(CopyPass, &Source, &Dest, false);

	SDL_EndGPUCopyPass(CopyPass);
	SDL_SubmitGPUCommandBuffer(UploadCommandBuffer);
	SDL_ReleaseGPUTransferBuffer(State.Device, TransferBuffer);
}

inline u32 PlaneVertexAt(i32 X, i32 Z, i32 Segements)
{
    return X * (Segements * 2 + 1) + Z;
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

    SDL_GPUShader *FragmentShader = LoadShader("assets/default.frag.spv", 1, true);
    SDL_GPUShader *VertexShader = LoadShader("assets/default.vert.spv", 1, false);

    // TODO: Make sure this format is actually available. Use fallback then!
    SDL_GPUTextureFormat DepthFormat = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;

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

    SDL_GPUVertexAttribute AttributeNormal = {};
    AttributeNormal.buffer_slot = 0;
    AttributeNormal.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
    AttributeNormal.location = 1;
    AttributeNormal.offset = offsetof(vertex, Normal);

    SDL_GPUVertexAttribute VertexAttributes[2] = { AttributePosition, AttributeNormal };

    SDL_GPUColorTargetDescription SwapchainTargetDescription = {};
    SwapchainTargetDescription.format = SDL_GetGPUSwapchainTextureFormat(State.Device, Window);

    SDL_GPUGraphicsPipelineCreateInfo PipelineInfo = {};
    PipelineInfo.vertex_shader = VertexShader;
    PipelineInfo.fragment_shader = FragmentShader;
    PipelineInfo.vertex_input_state.vertex_buffer_descriptions = &BufferDescription;
    PipelineInfo.vertex_input_state.num_vertex_buffers = 1;
    PipelineInfo.vertex_input_state.vertex_attributes = VertexAttributes;
    PipelineInfo.vertex_input_state.num_vertex_attributes = 2;

    PipelineInfo.depth_stencil_state.enable_depth_test = true;
    PipelineInfo.depth_stencil_state.enable_depth_write = true;
    PipelineInfo.depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_LESS;

    PipelineInfo.target_info.color_target_descriptions = &SwapchainTargetDescription;
    PipelineInfo.target_info.num_color_targets = 1;
    PipelineInfo.target_info.has_depth_stencil_target = true;
    PipelineInfo.target_info.depth_stencil_format = DepthFormat;

    PipelineInfo.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_LINE;

    SDL_GPUGraphicsPipeline *Pipeline =  SDL_CreateGPUGraphicsPipeline(State.Device, &PipelineInfo);
    assert(Pipeline);

    SDL_ReleaseGPUShader(State.Device, VertexShader);
	SDL_ReleaseGPUShader(State.Device, FragmentShader);

    // Depth Buffer
    //
    SDL_GPUTextureCreateInfo DepthBufferInfo = {};
    DepthBufferInfo.type = SDL_GPU_TEXTURETYPE_2D;
    DepthBufferInfo.format = DepthFormat;
    DepthBufferInfo.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
    DepthBufferInfo.width = WindowWidth;
    DepthBufferInfo.height = WindowHeight;
    DepthBufferInfo.layer_count_or_depth = 1;
    DepthBufferInfo.num_levels = 1;
    SDL_GPUTexture *DepthBuffer = SDL_CreateGPUTexture(State.Device, &DepthBufferInfo);

    u32 VertexCount = 0;
    vertex Vertices[1024] = {};

    u32 IndexCount = 0;
    u32 Indices[1024] = {};

    // Vertices[0].Position = V3(-0.3, -0.3, 0.0);
    // Vertices[0].Normal = V3(0, 0, 1);
    // Vertices[1].Position = V3(0.3, -0.3, 0.0);
    // Vertices[1].Normal = V3(0, 0, 1);
    // Vertices[2].Position = V3(0.0, 0.3, 0.0);
    // Vertices[2].Normal = V3(0, 0, 1);
    //
    // Vertices[3].Position = V3(-1.0, 0, -1.0);
    // Vertices[3].Normal = V3(0, 1, 0);
    // Vertices[4].Position = V3(1.0, 0, -1.0);
    // Vertices[4].Normal = V3(0, 1, 0);
    // Vertices[5].Position = V3(-1.0, 0, 1.0);
    // Vertices[5].Normal = V3(0, 1, 0);
    // Vertices[6].Position = V3(1.0, 0, 1.0);
    // Vertices[6].Normal = V3(0, 1, 0);
    //
    // VertexCount = 7;
    //
    // Indices[0] = 0;
    // Indices[1] = 1;
    // Indices[2] = 2;
    //
    // Indices[3] = 3;
    // Indices[4] = 4;
    // Indices[5] = 5;
    //
    // Indices[6] = 4;
    // Indices[7] = 5;
    // Indices[8] = 6;
    //
    // IndexCount = 9;

    i32 Segments = 5;

    for (i32 I = -Segments; I <= Segments; ++I)
    {
        for (i32 J = -Segments; J <= Segments; ++J)
        {
            f32 X = (f32) I / (f32) Segments;
            f32 Z = (f32) J / (f32) Segments;

            vertex *Vertex = Vertices + VertexCount++;
            Vertex->Position = V3(X, 0, Z);
            Vertex->Normal = V3(0, 1, 0);
        }
    }

    for (i32 I = 0; I < 2 * Segments; ++I)
    {
        for (i32 J = 0; J < 2 * Segments; ++J)
        {
            Indices[IndexCount + 0] = PlaneVertexAt(I, J, Segments);
            Indices[IndexCount + 1] = PlaneVertexAt(I + 1, J, Segments);
            Indices[IndexCount + 2] = PlaneVertexAt(I + 1, J + 1, Segments);

            Indices[IndexCount + 3] = PlaneVertexAt(I, J, Segments);
            Indices[IndexCount + 4] = PlaneVertexAt(I + 1, J + 1, Segments);
            Indices[IndexCount + 5] = PlaneVertexAt(I, J + 1, Segments);

            IndexCount += 6;
        }
    }

    // Vertex Buffer
    //
    SDL_GPUBufferCreateInfo VertexBufferInfo = {};
    VertexBufferInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    VertexBufferInfo.size = sizeof(vertex) * VertexCount;
    SDL_GPUBuffer *VertexBuffer = SDL_CreateGPUBuffer(State.Device, &VertexBufferInfo);

    CopyToBuffer(VertexBuffer, Vertices, sizeof(vertex) * VertexCount);

    // Index Buffer
    //
    SDL_GPUBufferCreateInfo IndexBufferInfo = {};
    IndexBufferInfo.usage = SDL_GPU_BUFFERUSAGE_INDEX;
    IndexBufferInfo.size = sizeof(u32) * IndexCount;
    SDL_GPUBuffer *IndexBuffer = SDL_CreateGPUBuffer(State.Device, &IndexBufferInfo);

    CopyToBuffer(IndexBuffer, Indices, sizeof(u32) * IndexCount);

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
        GlobalUniforms.View = LookAt(V3(0, 1, 1), V3(0), V3(0, 1, 0));

        // NOTE: When window is minimized there is no swapchain image, so SwapchainTexture will be NULL
        if (SwapchainTexture)
        {
            SDL_GPUColorTargetInfo ColorTargetInfo = {};
            ColorTargetInfo.texture = SwapchainTexture;
            ColorTargetInfo.clear_color = { 0.1f, 0.1f, 0.2f, 1.0f };
            ColorTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
            ColorTargetInfo.store_op = SDL_GPU_STOREOP_STORE;

            SDL_GPUDepthStencilTargetInfo DepthTargetInfo = {};
            DepthTargetInfo.texture = DepthBuffer;
            DepthTargetInfo.clear_depth = 1;
            DepthTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
            DepthTargetInfo.store_op = SDL_GPU_STOREOP_STORE;

            SDL_GPURenderPass *RenderPass = SDL_BeginGPURenderPass(CommandBuffer, &ColorTargetInfo, 1, &DepthTargetInfo);
            SDL_BindGPUGraphicsPipeline(RenderPass, Pipeline);

            SDL_GPUBufferBinding VertexBufferBinding = {};
            VertexBufferBinding.buffer = VertexBuffer;
            SDL_BindGPUVertexBuffers(RenderPass, 0, &VertexBufferBinding, 1);
            
            SDL_GPUBufferBinding IndexBufferBinding = {};
            IndexBufferBinding.buffer = IndexBuffer;
            SDL_BindGPUIndexBuffer(RenderPass, &IndexBufferBinding, SDL_GPU_INDEXELEMENTSIZE_32BIT);

            SDL_PushGPUVertexUniformData(CommandBuffer, 0, &GlobalUniforms, sizeof(GlobalUniforms));

            SDL_DrawGPUIndexedPrimitives(RenderPass, IndexCount, 1, 0, 0, 0);

            SDL_EndGPURenderPass(RenderPass);
        }

        SDL_SubmitGPUCommandBuffer(CommandBuffer);
    }
}
