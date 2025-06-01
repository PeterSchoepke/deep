package main

import sdl "vendor:sdl3"

last_frame_time :f32 = 0

main :: proc() {
	render_context: Render_Context
	camera: Camera
	lights: Lights
	meshes: Meshes

	create_window(&render_context)
	create_render_pipeline(&render_context)
	create_depth_buffer(&render_context)
	load_textures(&render_context)
	camera_init(&camera, {0.0, 0.0, 3.0})
	load_meshes(&render_context, &meshes)
	load_lights(&lights)

	ok := sdl.SetWindowRelativeMouseMode(render_context.window, true); assert(ok)

	game_loop: for {
		if !processEvents(&camera) do break game_loop

		// update game state

		render(&render_context, &camera, &meshes, &lights)
	}

	destroy_meshes(&render_context, &meshes)
	destroy_textures(&render_context)
	destroy_depth_buffer(&render_context)
	destroy_render_pipeline(&render_context)
	destroy_window(&render_context)
}

processEvents :: proc(camera: ^Camera) -> bool {
	current_time := f32(sdl.GetTicks()) / 1_000.0;
    delta_time := current_time - last_frame_time;
    last_frame_time = current_time;

	ev: sdl.Event
	for sdl.PollEvent(&ev) {
		#partial switch ev.type {
			case .QUIT:
				return false
			case .KEY_DOWN:
				if ev.key.scancode == .ESCAPE do return false
			case .MOUSE_MOTION:
				camera_process_mouse_movement(camera, ev.motion.xrel, ev.motion.yrel*-1, true);
		}
	}

	keyboardState := sdl.GetKeyboardState(nil);

    camera_process_keyboard(
        camera, 
        keyboardState[sdl.Scancode.W],
        keyboardState[sdl.Scancode.S],
        keyboardState[sdl.Scancode.A],
        keyboardState[sdl.Scancode.D],
        keyboardState[sdl.Scancode.SPACE],
        keyboardState[sdl.Scancode.LSHIFT],
        delta_time
    )

	return true
}