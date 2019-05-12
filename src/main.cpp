//////////////////////////////////////////////////////////////////////////

// Application renderer
#include "renderer/renderer.hpp"

// Application core
#include "core/window.hpp"

// Application miscellaneous
#include "miscellaneous/global_settings.hpp"

//////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[])
{
	// Prevent "unreferenced formal parameter" warning from triggering
	argc, argv;

	vkc::Window window;
	vkc::Renderer renderer;

	// Create a window
	window.Create(
		vkc::global_settings::default_window_width,
		vkc::global_settings::default_window_height,
		vkc::global_settings::window_title);

	// Register key callback
	window.OnKey([&window](int key, int action) {
		if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		{
			window.Stop();
		}
	});

	// Register resize callback
	window.OnResize([&renderer](int new_width, int new_height) {
		// Prevent "unreferenced formal parameter" warning from triggering
		new_width, new_height;

		renderer.TriggerFramebufferResized();
	});

	// Application initialization
	window.OnInitialization([&renderer, &window]() {
		renderer.Initialize(window);
	});

	// Application update
	window.OnUpdate([&renderer](double delta_time) {
		// Prevent "unreferenced formal parameter" warning from triggering
		delta_time;

		renderer.Update();
	});

	// Application rendering
	window.OnDraw([&renderer]() {
		renderer.Draw();
	});

	// Application clean-up
	window.OnShutDown([&renderer]() {
		// #TODO: No clean-up functions yet
	});

	// Application entry point
	window.EnterMainLoop();

    return 0;
}
