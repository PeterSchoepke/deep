package deep

import sdl "vendor:sdl3"

render_context: Render_Context
camera: Camera
lights: Lights
meshes: Meshes

start :: proc() {
    create_window(&render_context)
	create_render_pipeline(&render_context)
	create_depth_buffer(&render_context)
	load_textures(&render_context)
	camera_init({0.0, 0.0, 0.0})
}

stop :: proc() {
    destroy_meshes(&render_context, &meshes)
	destroy_textures(&render_context)
	destroy_depth_buffer(&render_context)
	destroy_render_pipeline(&render_context)
	destroy_window(&render_context)
}

last_frame_time :f32 = 0
delta_time :: proc() -> f32 {
	current_time := f32(sdl.GetTicks()) / 1_000.0
    delta_time := current_time - last_frame_time
    last_frame_time = current_time
	return delta_time
}

mouse_lock :: proc(is_active: bool) {
	ok := sdl.SetWindowRelativeMouseMode(render_context.window, is_active); assert(ok)
}