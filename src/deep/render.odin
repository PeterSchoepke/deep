package deep

import "core:fmt"
import "core:math"
import "core:math/linalg"
import "core:mem"
import "core:slice"
import "core:strings"
import "core:c"
import sdl "vendor:sdl3"
import "vendor:cgltf"

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
    vertex_code := #load("../../shaders/vertex.spv")
    fragment_code := #load("../../shaders/fragment.spv")

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
        rasterizer_state = {
            cull_mode = .BACK,
        },
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
    render_context.diffuse_map = load_texture(render_context, "ressources/images/diffuse.bmp")
    render_context.specular_map = load_texture(render_context, "ressources/images/specular.bmp")
    render_context.shininess_map = load_texture(render_context, "ressources/images/shininess.bmp")
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

load_gltf :: proc(render_context: ^Render_Context, render_data: ^Render_Data, model_filename : string,) {
    copyData :: proc(accessor: ^cgltf.accessor, dst: rawptr) {
        bufferView := accessor.buffer_view
        data := bufferView.data
        if data == nil {
            data = bufferView.buffer.data
        }
        mem.copy(dst, rawptr(uintptr(data) + uintptr(bufferView.offset)), int(bufferView.size))
    }

    filename: cstring = strings.clone_to_cstring(model_filename)
    options: cgltf.options
	data, result := cgltf.parse_file(options, filename)
    defer cgltf.free(data)
	if result != .success {
        panic("Failed to load gltf file!")
    }

    if cgltf.load_buffers(options, data, filename) != .success {
        panic("Failed to load buffer gltf file!")
    }

    if len(data.meshes) > 0 {
        for &primitive in data.meshes[0].primitives {  // Only support one mesh per file
            if primitive.type != .triangles {
                continue
            }

            indices := make([]u16, int(primitive.indices.count))
            defer delete(indices)
            if primitive.indices.component_type == .r_32u {
                copyData(primitive.indices, &indices[0])
            } else if primitive.indices.component_type == .r_16u {
                data := make([]u16, int(primitive.indices.count))
                copyData(primitive.indices, &data[0])
                for &indice, index in indices {
                    indice = u16(data[index])
                }
                delete(data)
            } else if primitive.indices.component_type == .r_8u {
                data := make([]u8, int(primitive.indices.count))
                copyData(primitive.indices, &data[0])
                for &indice, index in indices {
                    indice = u16(data[index])
                }
                delete(data)
            }

            vertices := make(#soa[]Vertex, primitive.attributes[0].data.count)
            defer delete(vertices)
            for &attribute in primitive.attributes {
                #partial switch attribute.type {
                case .position:
                    copyData(attribute.data, &vertices[0].pos)
                case .texcoord:
                    copyData(attribute.data, &vertices[0].uv)
                case .normal:
                    copyData(attribute.data, &vertices[0].normal)
                }
            }

            fmt.println(indices)
            fmt.println(vertices)
            create_render_data(render_context, render_data, vertices, indices)
        }
    }
}

create_render_data :: proc(render_context: ^Render_Context, render_data: ^Render_Data, soa_vertices: #soa[]Vertex, indices: []u16) {
    
    vertices := make([]Vertex, len(soa_vertices))
    defer delete(vertices)
    for i := 0; i < len(soa_vertices); i += 1 {
        vertices[i].pos = soa_vertices.pos[i];
        vertices[i].uv = soa_vertices.uv[i];
        vertices[i].normal = soa_vertices.normal[i];
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

render :: proc() {
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
        view = camera_get_view_matrix(),
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