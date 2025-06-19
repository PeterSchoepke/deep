package main

import sdl "vendor:sdl3"
import deep "deep"

main :: proc() {
	deep.start()
	defer deep.stop()

	setup()

	game_loop: for {
		delta_time := deep.delta_time()
		if !input(delta_time) do break game_loop

		update(delta_time);

		deep.render()
	}
}

setup :: proc(){
	deep.add_mesh("ressources/models/cube.glb", {0.0, 0.0, 0.0})
	deep.add_light({0.5, 0.5, 3.0})

	deep.set_camera_position({0.0, 0.0, 3.0});
	deep.mouse_lock(true)
}

input :: proc(delta_time: f32) -> bool {
	ev: sdl.Event
	for sdl.PollEvent(&ev) {
		#partial switch ev.type {
			case .QUIT:
				return false
			case .KEY_DOWN:
				if ev.key.scancode == .ESCAPE do return false
			case .MOUSE_MOTION:
				deep.camera_process_mouse_movement(ev.motion.xrel, ev.motion.yrel*-1, true);
		}
	}

	keyboardState := sdl.GetKeyboardState(nil);

    deep.camera_process_keyboard(
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

update :: proc(delta_time: f32) {
	deep.rotate_mesh(0, delta_time * 30.0)
}