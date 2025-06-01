package main

import "core:math"
import "core:math/linalg"
import "core:mem"
import "core:slice"
import "core:strings"
import "core:c"
import sdl "vendor:sdl3"

create_window :: proc(render_context: ^Render_Context) {
    ok := sdl.Init({.VIDEO}); assert(ok)
	render_context.window = sdl.CreateWindow("Deep", 640, 480, {}); assert(render_context.window != nil)
	render_context.device = sdl.CreateGPUDevice({.SPIRV}, true, nil); assert(render_context.device != nil)
	ok = sdl.ClaimWindowForGPUDevice(render_context.device, render_context.window); assert(ok)
}
destroy_window :: proc(render_context: ^Render_Context) {
    sdl.DestroyGPUDevice(render_context.device)
    sdl.DestroyWindow(render_context.window)
}

create_render_pipeline :: proc(render_context: ^Render_Context) {
    vertex_code := #load("../shaders/vertex.spv")
    fragment_code := #load("../shaders/fragment.spv")

    vertex_shader := sdl.CreateGPUShader(render_context.device, {
        code = raw_data(vertex_code),
        code_size = len(vertex_code),
        entrypoint = "main",
        format = { .SPIRV },
        stage = .VERTEX,
        num_samplers = 0,
        num_storage_buffers = 0,
        num_storage_textures = 0,
        num_uniform_buffers = 1
    })

    fragment_shader := sdl.CreateGPUShader(render_context.device, {
        code = raw_data(fragment_code),
        code_size = len(fragment_code),
        entrypoint = "main",
        format = { .SPIRV },
        stage = .FRAGMENT,
        num_samplers = 3,
        num_storage_buffers = 0,
        num_storage_textures = 0,
        num_uniform_buffers = 1
    })

    vertex_attributes := []sdl.GPUVertexAttribute {
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

    render_context.graphics_pipeline = sdl.CreateGPUGraphicsPipeline(render_context.device, {
        vertex_shader = vertex_shader,
        fragment_shader = fragment_shader,
        primitive_type = .TRIANGLELIST,
        vertex_input_state = {
            num_vertex_buffers = 1,
            vertex_buffer_descriptions = &(sdl.GPUVertexBufferDescription {
                slot = 0,
                input_rate = .VERTEX,
                instance_step_rate = 0,
                pitch = size_of(Vertex),
            }),
            num_vertex_attributes = u32(len(vertex_attributes)),
            vertex_attributes = raw_data(vertex_attributes),
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
                format = sdl.GetGPUSwapchainTextureFormat(render_context.device, render_context.window)
            }),
            has_depth_stencil_target = true,
            depth_stencil_format = .D16_UNORM,
        },
    })

    sdl.ReleaseGPUShader(render_context.device, vertex_shader)
    sdl.ReleaseGPUShader(render_context.device, fragment_shader)
}
destroy_render_pipeline :: proc(render_context: ^Render_Context) {
    sdl.ReleaseGPUGraphicsPipeline(render_context.device, render_context.graphics_pipeline)
}

create_depth_buffer :: proc(render_context: ^Render_Context) {
    sceneWidth, sceneHeight : i32
    sdl.GetWindowSizeInPixels(render_context.window, &sceneWidth, &sceneHeight)
    
    render_context.scene_depth_texture = sdl.CreateGPUTexture(render_context.device, {
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
destroy_depth_buffer :: proc(render_context: ^Render_Context) {
    sdl.ReleaseGPUTexture(render_context.device, render_context.scene_depth_texture)
}

load_textures :: proc(render_context: ^Render_Context) {
    create_sampler(render_context)
    render_context.diffuse_map = load_texture(render_context, "ressources/diffuse.bmp")
    render_context.specular_map = load_texture(render_context, "ressources/specular.bmp")
    render_context.shininess_map = load_texture(render_context, "ressources/shininess.bmp")
}
create_sampler :: proc(render_context: ^Render_Context) {
    render_context.sampler = sdl.CreateGPUSampler(render_context.device, {
        min_filter = .NEAREST,
        mag_filter = .NEAREST,
        mipmap_mode = .NEAREST,
        address_mode_u = .CLAMP_TO_EDGE,
        address_mode_v = .CLAMP_TO_EDGE,
        address_mode_w = .CLAMP_TO_EDGE,
    })
}
load_image :: proc(image_filename : string, desired_channels: int) -> ^sdl.Surface{
    result: ^sdl.Surface
    format :sdl.PixelFormat

    cstr := strings.clone_to_cstring(image_filename)
    result = sdl.LoadBMP(cstr)
    if (result == nil)
    {
        sdl.Log("Failed to load BMP: %s", sdl.GetError())
        return nil
    }

    if (desired_channels == 4)
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
load_texture :: proc(render_context: ^Render_Context, filename: string) -> ^sdl.GPUTexture {
    image_data := load_image(filename, 4)
        if (image_data == nil)
        {
            sdl.Log("Could not load image data!")
            return nil
        }

    texture := sdl.CreateGPUTexture(render_context.device, {
        type = .D2,
        format = .R8G8B8A8_UNORM,
        width = u32(image_data.w),
        height = u32(image_data.h),
        layer_count_or_depth = 1,
        num_levels = 1,
        usage = {.SAMPLER},
    })

    texture_transfer_buffer := sdl.CreateGPUTransferBuffer(
        render_context.device,
        {
            usage = .UPLOAD,
            size = u32(image_data.w * image_data.h * 4),
        }
    )

    texture_transfer_pointer: rawptr = sdl.MapGPUTransferBuffer(
        render_context.device,
        texture_transfer_buffer,
        false
    )
    sdl.memcpy(texture_transfer_pointer, image_data.pixels, uint(image_data.w * image_data.h * 4))
    sdl.UnmapGPUTransferBuffer(render_context.device, texture_transfer_buffer)

    command_buffer := sdl.AcquireGPUCommandBuffer(render_context.device)
    copy_pass := sdl.BeginGPUCopyPass(command_buffer)
    
    sdl.UploadToGPUTexture(
        copy_pass,
        {
            transfer_buffer = texture_transfer_buffer,
            offset = 0,
        },
        {
            texture = texture,
            w = u32(image_data.w),
            h = u32(image_data.h),
            d = 1,
        },
        false
    )

    sdl.EndGPUCopyPass(copy_pass)
    ok := sdl.SubmitGPUCommandBuffer(command_buffer); assert(ok)
    sdl.DestroySurface(image_data)
    sdl.ReleaseGPUTransferBuffer(render_context.device, texture_transfer_buffer)

    return texture
}
destroy_textures :: proc(render_context: ^Render_Context) {
    sdl.ReleaseGPUTexture(render_context.device, render_context.diffuse_map)
    sdl.ReleaseGPUTexture(render_context.device, render_context.specular_map)
    sdl.ReleaseGPUTexture(render_context.device, render_context.shininess_map)
    sdl.ReleaseGPUSampler(render_context.device, render_context.sampler)
}

create_render_data :: proc(render_context: ^Render_Context, render_data: ^Render_Data) {
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

    vertices_byte_size := len(vertices) * size_of(Vertex)
    indices_byte_size := len(indices) * size_of(u16)

    render_data.vertex_buffer = sdl.CreateGPUBuffer(render_context.device, {
        size = u32(vertices_byte_size),
        usage = {.VERTEX},
    })

    render_data.index_buffer = sdl.CreateGPUBuffer(render_context.device, {
        size = u32(indices_byte_size),
        usage = {.INDEX},
    })

    buffer_transfer_buffer := sdl.CreateGPUTransferBuffer(render_context.device, {
        size = u32(vertices_byte_size + indices_byte_size),
        usage = .UPLOAD,
    })

    transfer_data := transmute([^]byte)sdl.MapGPUTransferBuffer(render_context.device, buffer_transfer_buffer, false)
    mem.copy(transfer_data, raw_data(vertices), vertices_byte_size)
    mem.copy(transfer_data[(vertices_byte_size):], raw_data(indices), indices_byte_size)

    sdl.UnmapGPUTransferBuffer(render_context.device, buffer_transfer_buffer)

    command_buffer := sdl.AcquireGPUCommandBuffer(render_context.device)
    copy_pass := sdl.BeginGPUCopyPass(command_buffer)

    sdl.UploadToGPUBuffer(copy_pass, {
        transfer_buffer = buffer_transfer_buffer,
        offset = 0,
    }, {
        buffer = render_data.vertex_buffer,
        size = u32(vertices_byte_size),
        offset = 0,
    }, false)

    sdl.UploadToGPUBuffer(copy_pass, {
        transfer_buffer = buffer_transfer_buffer,
        offset = u32(vertices_byte_size),
    }, {
        buffer = render_data.index_buffer,
        size = u32(indices_byte_size),
        offset = 0,
    }, false)

    sdl.EndGPUCopyPass(copy_pass)
    ok := sdl.SubmitGPUCommandBuffer(command_buffer); assert(ok)
    sdl.ReleaseGPUTransferBuffer(render_context.device, buffer_transfer_buffer)

    render_data.transform = linalg.MATRIX4F32_IDENTITY
}
destroy_render_data :: proc(render_context: ^Render_Context, render_data: ^Render_Data) {
    sdl.ReleaseGPUBuffer(render_context.device, render_data.vertex_buffer)
    sdl.ReleaseGPUBuffer(render_context.device, render_data.index_buffer)
}

render :: proc(render_context: ^Render_Context, camera: ^Camera, meshes: ^Meshes, lights: ^Lights) {
    command_buffer := sdl.AcquireGPUCommandBuffer(render_context.device)
    if command_buffer == nil {
        return
    }

    swapchain_texture: ^sdl.GPUTexture
    width, height: u32
    if !sdl.WaitAndAcquireGPUSwapchainTexture(command_buffer, render_context.window, &swapchain_texture, &width, &height) {
        return
    }

    color_target_info: sdl.GPUColorTargetInfo = {
        clear_color = {0/255.0, 0/255.0, 0/255.0, 255/255.0},
        load_op = .CLEAR,
        store_op = .STORE,
        texture = swapchain_texture,
    }    

    depth_stencil_target_info: sdl.GPUDepthStencilTargetInfo = {
        texture = render_context.scene_depth_texture,
        cycle = true,
        clear_depth = 1,
        clear_stencil = 0,
        load_op = .CLEAR,
        store_op = .STORE,
        stencil_load_op = .CLEAR,
        stencil_store_op = .STORE,
    }    

    render_pass := sdl.BeginGPURenderPass(command_buffer, &color_target_info, 1, &depth_stencil_target_info)

    sdl.BindGPUGraphicsPipeline(render_pass, render_context.graphics_pipeline)

    vertex_uniform_buffer: VertexUniformBuffer = {
        view = camera_get_view_matrix(camera),
        projection = camera.projection,
    }    

    fragment_uniform_buffer: FragmentUniformBuffer
    for i in 0..<lights.count {
        fragment_uniform_buffer.lights[i].position = lights.data[i]
        fragment_uniform_buffer.lights[i].ambient = Vec3{0.2, 0.2, 0.2}
        fragment_uniform_buffer.lights[i].diffuse = Vec3{0.5, 0.5, 0.5}
        fragment_uniform_buffer.lights[i].specular = Vec3{1.0, 1.0, 1.0}
        fragment_uniform_buffer.lights[i].constant_linear_quadratic = Vec3{1.0, 0.09, 0.032}
    }
    fragment_uniform_buffer.number_of_lights = i32(lights.count)
    fragment_uniform_buffer.camera_position = camera.position
    sdl.PushGPUFragmentUniformData(command_buffer, 0, &fragment_uniform_buffer, size_of(FragmentUniformBuffer))

    texture_sampler_bindings: [4]sdl.GPUTextureSamplerBinding
    texture_sampler_bindings[0].texture = render_context.diffuse_map
    texture_sampler_bindings[0].sampler = render_context.sampler
    texture_sampler_bindings[1].texture = render_context.specular_map
    texture_sampler_bindings[1].sampler = render_context.sampler
    texture_sampler_bindings[2].texture = render_context.shininess_map
    texture_sampler_bindings[2].sampler = render_context.sampler
    texture_sampler_bindings[3].texture = render_context.scene_depth_texture
    texture_sampler_bindings[3].sampler = render_context.sampler
    sdl.BindGPUFragmentSamplers(render_pass, 0, &texture_sampler_bindings[0], 3)

    for i in 0..<meshes.count {
        vertex_uniform_buffer.model = meshes.data[i].transform
        sdl.PushGPUVertexUniformData(command_buffer, 0, &vertex_uniform_buffer, size_of(VertexUniformBuffer))

        sdl.BindGPUVertexBuffers(render_pass, 0, &sdl.GPUBufferBinding{
            buffer = meshes.data[i].vertex_buffer,
            offset = 0,
        }, 1)

        sdl.BindGPUIndexBuffer(render_pass, {
            buffer = meshes.data[i].index_buffer,
            offset = 0,
        }, ._16BIT)

        sdl.DrawGPUIndexedPrimitives(render_pass, 36, 1, 0, 0, 0)
    }

    sdl.EndGPURenderPass(render_pass)

    ok := sdl.SubmitGPUCommandBuffer(command_buffer); assert(ok)
}