package main

import sdl "vendor:sdl3"

lastFrameTime :f32 = 0

main :: proc() {
	renderContext: RenderContext
	camera: Camera
	lights: Lights
	meshes: Meshes

	createWindow(&renderContext)
	createRenderPipeline(&renderContext)
	createDepthBuffer(&renderContext)
	loadTextures(&renderContext)
	cameraInit(&camera, {0.0, 0.0, 3.0})
	loadMeshes(&renderContext, &meshes)
	loadLights(&lights)

	ok := sdl.SetWindowRelativeMouseMode(renderContext.window, true); assert(ok)

	game_loop: for {
		if !processEvents(&camera) do break game_loop

		// update game state

		render(&renderContext, &camera, &meshes, &lights)
	}

	destroyMeshes(&renderContext, &meshes)
	destroyTextures(&renderContext)
	destroyDepthBuffer(&renderContext)
	destroyRenderPipeline(&renderContext)
	destroyWindow(&renderContext)
}

processEvents :: proc(camera: ^Camera) -> bool {
	currentTime := f32(sdl.GetTicks()) / 1_000.0;
    deltaTime := currentTime - lastFrameTime;
    lastFrameTime = currentTime;

	ev: sdl.Event
	for sdl.PollEvent(&ev) {
		#partial switch ev.type {
			case .QUIT:
				return false
			case .KEY_DOWN:
				if ev.key.scancode == .ESCAPE do return false
			case .MOUSE_MOTION:
				cameraProcessMouseMovement(camera, ev.motion.xrel, ev.motion.yrel*-1, true);
		}
	}

	keyboardState := sdl.GetKeyboardState(nil);

    cameraProcessKeyboard(
        camera, 
        keyboardState[sdl.Scancode.W],
        keyboardState[sdl.Scancode.S],
        keyboardState[sdl.Scancode.A],
        keyboardState[sdl.Scancode.D],
        keyboardState[sdl.Scancode.SPACE],
        keyboardState[sdl.Scancode.LSHIFT],
        deltaTime
    )

	return true
}