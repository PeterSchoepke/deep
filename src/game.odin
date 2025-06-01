package main

import "core:math"
import "core:math/linalg"

loadMeshes :: proc(render_context: ^RenderContext, meshes: ^Meshes) {
    cube_positions := [10]Vec3 {
        { 0.0,  0.0,   0.0},
        { 2.0,  5.0, -15.0},
        {-1.5, -2.2,  -2.5},
        {-3.8, -2.0, -12.3},
        { 2.4, -0.4,  -3.5},
        {-1.7,  3.0,  -7.5},
        { 1.3, -2.0,  -2.5},
        { 1.5,  2.0,  -2.5},
        { 1.5,  0.2,  -1.5},
        { 1.2,  1.0,   2.0},
    }

    for i in 0..<10 {
        createRenderData(render_context, &meshes.data[i])

        meshes.data[i].transform = linalg.MATRIX4F32_IDENTITY
        meshes.data[i].transform = linalg.matrix4_translate_f32(cube_positions[i]) * meshes.data[i].transform
        angle := f32(20.0 * f32(i))
        meshes.data[i].transform = linalg.matrix4_rotate(math.to_radians(angle), Vec3{1.0, 0.3, 0.5}) * meshes.data[i].transform
    }
    meshes.data[9].transform = linalg.matrix4_scale(Vec3{0.1, 0.1, 0.1}) * meshes.data[9].transform
    meshes.count = 10
}

destroyMeshes :: proc(render_context: ^RenderContext, meshes: ^Meshes) {
    for i in 0..<meshes.count {
        destroyRenderData(render_context, &meshes.data[i])
    }
}

loadLights :: proc(lights: ^Lights) {
    point_light_positions := [4]Vec3 {
        { 0.7,  0.2,   2.0},
        { 2.3, -3.3,  -4.0},
        {-4.0,  2.0, -12.0},
        { 0.0,  0.0,  -3.0},
    }

    for i in 0..<4 {
        lights.data[i] = point_light_positions[i]
    }
    lights.count = 4
}