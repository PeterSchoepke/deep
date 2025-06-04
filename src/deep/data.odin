package deep

import "core:fmt"
import "core:math"
import "core:slice"
import "core:math/linalg"
import sdl "vendor:sdl3"

Vec3 :: linalg.Vector3f32
Vec2 :: linalg.Vector2f32
Vec4 :: linalg.Vector4f32
Mat4 :: linalg.Matrix4f32
Quat :: quaternion128

Render_Context :: struct {
    window: ^sdl.Window,
    device: ^sdl.GPUDevice,
    graphics_pipeline: ^sdl.GPUGraphicsPipeline,

    diffuse_map: ^sdl.GPUTexture,
    specular_map: ^sdl.GPUTexture,
    shininess_map: ^sdl.GPUTexture,
    sampler: ^sdl.GPUSampler,
    scene_depth_texture: ^sdl.GPUTexture,
}

Vertex :: struct {
	pos: Vec3,
	uv: Vec2,
	normal: Vec3,
}

Render_Data :: struct {
    vertex_buffer: ^sdl.GPUBuffer,
    index_buffer: ^sdl.GPUBuffer,
    index_count: u32,
    transform: Mat4,
}

Meshes :: struct {
    data: [10]Render_Data,
    count: int,
}

Lights :: struct {
    data: [10]Vec3,
    count: int,
}

Light_Data :: struct {
    position: Vec3,
    _: f32,

    ambient: Vec3,
    _: f32,
    diffuse: Vec3,
    _: f32,
    specular: Vec3,
    _: f32,

    constant_linear_quadratic: Vec3,
    _: f32,
}

VertexUniformBuffer :: struct #packed {
    model: Mat4,
    view: Mat4,
    projection: Mat4,
}

FragmentUniformBuffer :: struct #packed {
    camera_position: Vec3,
    _: f32,
    lights: [10]Light_Data,
    number_of_lights: i32,
}

Camera :: struct {
    position: Vec3,
    front: Vec3,
    up: Vec3,
    right: Vec3,
    world_up: Vec3,

    yaw: f32,
    pitch: f32,
    
    movement_speed: f32,
    mouse_sensitivity: f32,
    
    projection: Mat4,
}