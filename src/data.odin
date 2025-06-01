package main

import "core:fmt"
import "core:math"
import "core:slice"
import "core:math/linalg"
import sdl "vendor:sdl3"

Vec3 :: linalg.Vector3f32
Vec2 :: linalg.Vector2f32
Mat4 :: linalg.Matrix4f32
Quat :: quaternion128

RenderContext :: struct {
    window: ^sdl.Window,
    device: ^sdl.GPUDevice,
    graphicsPipeline: ^sdl.GPUGraphicsPipeline,

    diffuseMap: ^sdl.GPUTexture,
    specularMap: ^sdl.GPUTexture,
    shininessMap: ^sdl.GPUTexture,
    sampler: ^sdl.GPUSampler,
    sceneDepthTexture: ^sdl.GPUTexture,
}

Vertex :: struct {
	pos: Vec3,
	uv: Vec2,
	normal: Vec3,
}

RenderData :: struct {
    vertexBuffer: ^sdl.GPUBuffer,
    indexBuffer: ^sdl.GPUBuffer,
    transform: Mat4,
}

Meshes :: struct {
    data: [10]RenderData,
    count: int,
}

Lights :: struct {
    data: [10]Vec3,
    count: int,
}

LightData :: struct {
    position: Vec3,
    _: f32,

    ambient: Vec3,
    _: f32,
    diffuse: Vec3,
    _: f32,
    specular: Vec3,
    _: f32,

    constantLinearQuadratic: Vec3,
    _: f32,
}

VertexUniformBuffer :: struct #packed {
    model: Mat4,
    view: Mat4,
    projection: Mat4,
}

FragmentUniformBuffer :: struct #packed {
    cameraPosition: Vec3,
    _: f32,
    lights: [10]LightData, // Matches Lights struct capacity
    numberOfLights: i32,
}

Camera :: struct {
    position: Vec3,
    front: Vec3,
    up: Vec3,
    right: Vec3,
    worldUp: Vec3,

    yaw: f32,
    pitch: f32,
    
    movementSpeed: f32,
    mouseSensitivity: f32,
    
    projection: Mat4,
}