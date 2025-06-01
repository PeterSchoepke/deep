package main

import "core:math"
import "core:math/linalg"
import "core:mem"
import "core:slice"
import "core:strings"
import "core:c"
import sdl "vendor:sdl3"

createWindow :: proc(renderContext: ^RenderContext) {
    ok := sdl.Init({.VIDEO}); assert(ok)
	renderContext.window = sdl.CreateWindow("Deep", 640, 480, {}); assert(renderContext.window != nil)
	renderContext.device = sdl.CreateGPUDevice({.SPIRV}, true, nil); assert(renderContext.device != nil)
	ok = sdl.ClaimWindowForGPUDevice(renderContext.device, renderContext.window); assert(ok)
}
destroyWindow :: proc(renderContext: ^RenderContext) {
    sdl.DestroyGPUDevice(renderContext.device)
    sdl.DestroyWindow(renderContext.window)
}

createRenderPipeline :: proc(renderContext: ^RenderContext) {
    vertexCode := #load("../shaders/vertex.spv")
    fragmentCode := #load("../shaders/fragment.spv")

    vertexShader := sdl.CreateGPUShader(renderContext.device, {
        code = raw_data(vertexCode),
        code_size = len(vertexCode),
        entrypoint = "main",
        format = { .SPIRV },
        stage = .VERTEX,
        num_samplers = 0,
        num_storage_buffers = 0,
        num_storage_textures = 0,
        num_uniform_buffers = 1
    })

    fragmentShader := sdl.CreateGPUShader(renderContext.device, {
        code = raw_data(fragmentCode),
        code_size = len(fragmentCode),
        entrypoint = "main",
        format = { .SPIRV },
        stage = .FRAGMENT,
        num_samplers = 3,
        num_storage_buffers = 0,
        num_storage_textures = 0,
        num_uniform_buffers = 1
    })

    vertex_attrs := []sdl.GPUVertexAttribute {
        {
            location = 0,
            buffer_slot = 0,
            format = .FLOAT3,
            offset = u32(offset_of(Vertex, pos)),
        },
        {
            location = 1,
            buffer_slot = 0,
            format = .FLOAT2,
            offset = u32(offset_of(Vertex, uv)),
        },
        {
            location = 2,
            buffer_slot = 0,
            format = .FLOAT3,
            offset = u32(offset_of(Vertex, normal)),
        }
    }

    renderContext.graphicsPipeline = sdl.CreateGPUGraphicsPipeline(renderContext.device, {
        vertex_shader = vertexShader,
        fragment_shader = fragmentShader,
        primitive_type = .TRIANGLELIST,
        vertex_input_state = {
            num_vertex_buffers = 1,
            vertex_buffer_descriptions = &(sdl.GPUVertexBufferDescription {
                slot = 0,
                input_rate = .VERTEX,
                instance_step_rate = 0,
                pitch = size_of(Vertex),
            }),
            num_vertex_attributes = u32(len(vertex_attrs)),
            vertex_attributes = raw_data(vertex_attrs),
        },
        depth_stencil_state = {
            enable_depth_test = true,
            enable_depth_write = true,
            compare_op = .LESS,
        },
        /*rasterizer_state = {
            cull_mode = .BACK,
        },*/
        target_info = {
            num_color_targets = 1,
            color_target_descriptions = &(sdl.GPUColorTargetDescription {
                format = sdl.GetGPUSwapchainTextureFormat(renderContext.device, renderContext.window)
            }),
            has_depth_stencil_target = true,
            depth_stencil_format = .D16_UNORM,
        },
    })

    sdl.ReleaseGPUShader(renderContext.device, vertexShader)
    sdl.ReleaseGPUShader(renderContext.device, fragmentShader)
}
destroyRenderPipeline :: proc(renderContext: ^RenderContext) {
    sdl.ReleaseGPUGraphicsPipeline(renderContext.device, renderContext.graphicsPipeline)
}

createDepthBuffer :: proc(renderContext: ^RenderContext) {
    sceneWidth, sceneHeight : i32
    sdl.GetWindowSizeInPixels(renderContext.window, &sceneWidth, &sceneHeight)
    
    renderContext.sceneDepthTexture = sdl.CreateGPUTexture(renderContext.device, {
        type = .D2,
        width = u32(sceneWidth),
        height = u32(sceneHeight),
        layer_count_or_depth = 1,
        num_levels = 1,
        sample_count = ._1,
        format = .D16_UNORM,
        usage = {.DEPTH_STENCIL_TARGET},
    })
}
destroyDepthBuffer :: proc(renderContext: ^RenderContext) {
    sdl.ReleaseGPUTexture(renderContext.device, renderContext.sceneDepthTexture)
}

loadTextures :: proc(renderContext: ^RenderContext) {
    createSampler(renderContext)
    renderContext.diffuseMap = loadTexture(renderContext, "ressources/diffuse.bmp")
    renderContext.specularMap = loadTexture(renderContext, "ressources/specular.bmp")
    renderContext.shininessMap = loadTexture(renderContext, "ressources/shininess.bmp")
}
createSampler :: proc(renderContext: ^RenderContext) {
    renderContext.sampler = sdl.CreateGPUSampler(renderContext.device, {
        min_filter = .NEAREST,
        mag_filter = .NEAREST,
        mipmap_mode = .NEAREST,
        address_mode_u = .CLAMP_TO_EDGE,
        address_mode_v = .CLAMP_TO_EDGE,
        address_mode_w = .CLAMP_TO_EDGE,
    })
}
loadImage :: proc(imageFilename : string, desiredChannels: int) -> ^sdl.Surface{
    result: ^sdl.Surface
    format :sdl.PixelFormat

    cstr := strings.clone_to_cstring(imageFilename)
    result = sdl.LoadBMP(cstr)
    if (result == nil)
    {
        sdl.Log("Failed to load BMP: %s", sdl.GetError())
        return nil
    }

    if (desiredChannels == 4)
    {
        format = .ABGR8888
    }
    else
    {
        sdl.DestroySurface(result)
        return nil
    }
    if (result.format != format)
    {
        next: ^sdl.Surface = sdl.ConvertSurface(result, format)
        sdl.DestroySurface(result)
        result = next
    }

    return result
}
loadTexture :: proc(renderContext: ^RenderContext, filename: string) -> ^sdl.GPUTexture {
    imageData := loadImage(filename, 4)
        if (imageData == nil)
        {
            sdl.Log("Could not load image data!")
            return nil
        }

    texture := sdl.CreateGPUTexture(renderContext.device, {
        type = .D2,
        format = .R8G8B8A8_UNORM,
        width = u32(imageData.w),
        height = u32(imageData.h),
        layer_count_or_depth = 1,
        num_levels = 1,
        usage = {.SAMPLER},
    })

    textureTransferBuffer := sdl.CreateGPUTransferBuffer(
        renderContext.device,
        {
            usage = .UPLOAD,
            size = u32(imageData.w * imageData.h * 4),
        }
    )

    textureTransferPtr: rawptr = sdl.MapGPUTransferBuffer(
        renderContext.device,
        textureTransferBuffer,
        false
    )
    sdl.memcpy(textureTransferPtr, imageData.pixels, uint(imageData.w * imageData.h * 4))
    sdl.UnmapGPUTransferBuffer(renderContext.device, textureTransferBuffer)

    // start a copy pass
    commandBuffer := sdl.AcquireGPUCommandBuffer(renderContext.device)
    copyPass := sdl.BeginGPUCopyPass(commandBuffer)
    
    sdl.UploadToGPUTexture(
        copyPass,
        {
            transfer_buffer = textureTransferBuffer,
            offset = 0, /* Zeros out the rest */
        },
        {
            texture = texture,
            w = u32(imageData.w),
            h = u32(imageData.h),
            d = 1,
        },
        false
    )

    sdl.EndGPUCopyPass(copyPass)
    ok := sdl.SubmitGPUCommandBuffer(commandBuffer); assert(ok)
    sdl.DestroySurface(imageData)
    sdl.ReleaseGPUTransferBuffer(renderContext.device, textureTransferBuffer)

    return texture
}
destroyTextures :: proc(renderContext: ^RenderContext) {
    sdl.ReleaseGPUTexture(renderContext.device, renderContext.diffuseMap)
    sdl.ReleaseGPUTexture(renderContext.device, renderContext.specularMap)
    sdl.ReleaseGPUTexture(renderContext.device, renderContext.shininessMap)
    sdl.ReleaseGPUSampler(renderContext.device, renderContext.sampler)
}

createRenderData :: proc(render_context: ^RenderContext, render_data: ^RenderData) {
    vertices := []Vertex {
        // Front face (Z = 0.5), Normal: (0.0, 0.0, 1.0)
        {{-0.5, -0.5,  0.5}, {0.0, 0.0}, {0.0, 0.0, 1.0}}, // 0
        {{ 0.5, -0.5,  0.5}, {1.0, 0.0}, {0.0, 0.0, 1.0}}, // 1
        {{ 0.5,  0.5,  0.5}, {1.0, 1.0}, {0.0, 0.0, 1.0}}, // 2
        {{-0.5,  0.5,  0.5}, {0.0, 1.0}, {0.0, 0.0, 1.0}}, // 3

        // Back face (Z = -0.5), Normal: (0.0, 0.0, -1.0)
        {{-0.5, -0.5, -0.5}, {1.0, 0.0}, {0.0, 0.0, -1.0}}, // 4
        {{ 0.5, -0.5, -0.5}, {0.0, 0.0}, {0.0, 0.0, -1.0}}, // 5
        {{ 0.5,  0.5, -0.5}, {0.0, 1.0}, {0.0, 0.0, -1.0}}, // 6
        {{-0.5,  0.5, -0.5}, {1.0, 1.0}, {0.0, 0.0, -1.0}}, // 7

        // Top face (Y = 0.5), Normal: (0.0, 1.0, 0.0)
        {{-0.5,  0.5,  0.5}, {0.0, 1.0}, {0.0, 1.0, 0.0}}, // 8 (same pos as 3)
        {{ 0.5,  0.5,  0.5}, {1.0, 1.0}, {0.0, 1.0, 0.0}}, // 9 (same pos as 2)
        {{ 0.5,  0.5, -0.5}, {1.0, 0.0}, {0.0, 1.0, 0.0}}, // 10 (same pos as 6)
        {{-0.5,  0.5, -0.5}, {0.0, 0.0}, {0.0, 1.0, 0.0}}, // 11 (same pos as 7)

        // Bottom face (Y = -0.5), Normal: (0.0, -1.0, 0.0)
        {{-0.5, -0.5,  0.5}, {0.0, 0.0}, {0.0, -1.0, 0.0}}, // 12 (same pos as 0)
        {{ 0.5, -0.5,  0.5}, {1.0, 0.0}, {0.0, -1.0, 0.0}}, // 13 (same pos as 1)
        {{ 0.5, -0.5, -0.5}, {1.0, 1.0}, {0.0, -1.0, 0.0}}, // 14 (same pos as 5)
        {{-0.5, -0.5, -0.5}, {0.0, 1.0}, {0.0, -1.0, 0.0}}, // 15 (same pos as 4)

        // Right face (X = 0.5), Normal: (1.0, 0.0, 0.0)
        {{ 0.5, -0.5,  0.5}, {0.0, 0.0}, {1.0, 0.0, 0.0}}, // 16 (same pos as 1)
        {{ 0.5, -0.5, -0.5}, {1.0, 0.0}, {1.0, 0.0, 0.0}}, // 17 (same pos as 5)
        {{ 0.5,  0.5, -0.5}, {1.0, 1.0}, {1.0, 0.0, 0.0}}, // 18 (same pos as 6)
        {{ 0.5,  0.5,  0.5}, {0.0, 1.0}, {1.0, 0.0, 0.0}}, // 19 (same pos as 2)

        // Left face (X = -0.5), Normal: (-1.0, 0.0, 0.0)
        {{-0.5, -0.5,  0.5}, {1.0, 0.0}, {-1.0, 0.0, 0.0}}, // 20 (same pos as 0)
        {{-0.5, -0.5, -0.5}, {0.0, 0.0}, {-1.0, 0.0, 0.0}}, // 21 (same pos as 4)
        {{-0.5,  0.5, -0.5}, {0.0, 1.0}, {-1.0, 0.0, 0.0}}, // 22 (same pos as 7)
        {{-0.5,  0.5,  0.5}, {1.0, 1.0}, {-1.0, 0.0, 0.0}}, // 23 (same pos as 3)
    }

    indices := []u16 {
        // Front face
        0, 1, 2,  2, 3, 0,
        // Back face
        4, 5, 6,  6, 7, 4,
        // Top face
        8, 9, 10,  10, 11, 8,
        // Bottom face
        12, 13, 14,  14, 15, 12,
        // Right face
        16, 17, 18,  18, 19, 16,
        // Left face
        20, 21, 22,  22, 23, 20,
    }

    verticesByteSize := len(vertices) * size_of(Vertex)
    indicesByteSize := len(indices) * size_of(u16)

    // Create the vertex buffer
    render_data.vertexBuffer = sdl.CreateGPUBuffer(render_context.device, {
        size = u32(verticesByteSize),
        usage = {.VERTEX},
    })

    // Create the index buffer
    render_data.indexBuffer = sdl.CreateGPUBuffer(render_context.device, {
        size = u32(indicesByteSize),
        usage = {.INDEX},
    })

    // Create a transfer buffer to upload to the vertex and index buffers
    buffer_transfer_buffer := sdl.CreateGPUTransferBuffer(render_context.device, {
        size = u32(verticesByteSize + indicesByteSize),
        usage = .UPLOAD,
    })

    // Fill the transfer buffer
    verticesCount := 24;
    transferData := transmute([^]byte)sdl.MapGPUTransferBuffer(render_context.device, buffer_transfer_buffer, false)
    mem.copy(transferData, raw_data(vertices), verticesByteSize)
    mem.copy(transferData[(verticesByteSize):], raw_data(indices), indicesByteSize)

    sdl.UnmapGPUTransferBuffer(render_context.device, buffer_transfer_buffer)

    // Start a copy pass
    command_buffer := sdl.AcquireGPUCommandBuffer(render_context.device)
    copy_pass := sdl.BeginGPUCopyPass(command_buffer)

    // Upload the vertex buffer
    sdl.UploadToGPUBuffer(copy_pass, {
        transfer_buffer = buffer_transfer_buffer,
        offset = 0,
    }, {
        buffer = render_data.vertexBuffer,
        size = u32(verticesByteSize),
        offset = 0,
    }, false)

    // Upload the index buffer
    sdl.UploadToGPUBuffer(copy_pass, {
        transfer_buffer = buffer_transfer_buffer,
        offset = u32(verticesByteSize),
    }, {
        buffer = render_data.indexBuffer,
        size = u32(indicesByteSize),
        offset = 0,
    }, false)

    // End the copy pass
    sdl.EndGPUCopyPass(copy_pass)
    ok := sdl.SubmitGPUCommandBuffer(command_buffer); assert(ok)
    sdl.ReleaseGPUTransferBuffer(render_context.device, buffer_transfer_buffer)

    // Initialize transform (identity matrix, with a rotation as in original code)
    render_data.transform = linalg.MATRIX4F32_IDENTITY
    //render_data.transform = linalg.matrix4_rotate_f32(f32(linalg.to_radians(-55.0)), Vec3{1.0, 0.0, 0.0}) * render_data.transform
}
destroyRenderData :: proc(render_context: ^RenderContext, render_data: ^RenderData) {
    sdl.ReleaseGPUBuffer(render_context.device, render_data.vertexBuffer)
    sdl.ReleaseGPUBuffer(render_context.device, render_data.indexBuffer)
}

render :: proc(renderContext: ^RenderContext, camera: ^Camera, meshes: ^Meshes, lights: ^Lights) {
    // Acquire the command buffer
    commandBuffer := sdl.AcquireGPUCommandBuffer(renderContext.device)
    if commandBuffer == nil {
        return
    }

    // Get the swapchain texture
    swapchainTexture: ^sdl.GPUTexture
    width, height: u32
    if !sdl.WaitAndAcquireGPUSwapchainTexture(commandBuffer, renderContext.window, &swapchainTexture, &width, &height) {
        return
    }

    // Create the color target
    colorTargetInfo: sdl.GPUColorTargetInfo = {
        clear_color = {0/255.0, 0/255.0, 0/255.0, 255/255.0},
        load_op = .CLEAR,
        store_op = .STORE,
        texture = swapchainTexture,
    }    

    // Create the depth-stencil target
    depthStencilTargetInfo: sdl.GPUDepthStencilTargetInfo = {
        texture = renderContext.sceneDepthTexture,
        cycle = true,
        clear_depth = 1,
        clear_stencil = 0,
        load_op = .CLEAR,
        store_op = .STORE,
        stencil_load_op = .CLEAR,
        stencil_store_op = .STORE,
    }    

    // Begin a render pass
    renderPass := sdl.BeginGPURenderPass(commandBuffer, &colorTargetInfo, 1, &depthStencilTargetInfo)

    // Bind the pipeline
    sdl.BindGPUGraphicsPipeline(renderPass, renderContext.graphicsPipeline)

    // Set up vertex uniform buffer
    vertexUniformBuffer: VertexUniformBuffer
    vertexUniformBuffer.view = cameraGetViewMatrix(camera)
    vertexUniformBuffer.projection = camera.projection

    // Set up fragment uniform buffer
    fragmentUniformBuffer: FragmentUniformBuffer
    for i in 0..<lights.count {
        fragmentUniformBuffer.lights[i].position = lights.data[i]
        fragmentUniformBuffer.lights[i].ambient = Vec3{0.2, 0.2, 0.2}
        fragmentUniformBuffer.lights[i].diffuse = Vec3{0.5, 0.5, 0.5}
        fragmentUniformBuffer.lights[i].specular = Vec3{1.0, 1.0, 1.0}
        fragmentUniformBuffer.lights[i].constantLinearQuadratic = Vec3{1.0, 0.09, 0.032}
    }
    fragmentUniformBuffer.numberOfLights = i32(lights.count)
    fragmentUniformBuffer.cameraPosition = camera.position
    sdl.PushGPUFragmentUniformData(commandBuffer, 0, &fragmentUniformBuffer, size_of(FragmentUniformBuffer))

    // Bind texture samplers
    textureSamplerBindings: [4]sdl.GPUTextureSamplerBinding
    textureSamplerBindings[0].texture = renderContext.diffuseMap
    textureSamplerBindings[0].sampler = renderContext.sampler
    textureSamplerBindings[1].texture = renderContext.specularMap
    textureSamplerBindings[1].sampler = renderContext.sampler
    textureSamplerBindings[2].texture = renderContext.shininessMap
    textureSamplerBindings[2].sampler = renderContext.sampler
    textureSamplerBindings[3].texture = renderContext.sceneDepthTexture
    textureSamplerBindings[3].sampler = renderContext.sampler
    sdl.BindGPUFragmentSamplers(renderPass, 0, &textureSamplerBindings[0], 3)

    for i in 0..<meshes.count {
        vertexUniformBuffer.model = meshes.data[i].transform
        sdl.PushGPUVertexUniformData(commandBuffer, 0, &vertexUniformBuffer, size_of(VertexUniformBuffer))

        vertexBufferBinding: sdl.GPUBufferBinding = {
            buffer = meshes.data[i].vertexBuffer,
            offset = 0,
        }
        sdl.BindGPUVertexBuffers(renderPass, 0, &vertexBufferBinding, 1)

        indexBufferBinding: sdl.GPUBufferBinding = {
            buffer = meshes.data[i].indexBuffer,
            offset = 0,
        }
        sdl.BindGPUIndexBuffer(renderPass, indexBufferBinding, ._16BIT)

        sdl.DrawGPUIndexedPrimitives(renderPass, 36, 1, 0, 0, 0)
    }

    sdl.EndGPURenderPass(renderPass)

    ok := sdl.SubmitGPUCommandBuffer(commandBuffer); assert(ok)
}