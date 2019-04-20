//////////////////////////////////////////////////////////////////////////

// Renderer
#include "renderer/renderer.hpp"

//////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[])
{
	// Prevent "unreferenced formal parameter" warning from triggering
	argc, argv;

	vkc::Renderer renderer;
	renderer.InitializeVulkan();
	renderer.SetupWindow(1280, 720, "Vulkanic");

	auto* const window_handle = renderer.GetHandle();
	
	glfwSetKeyCallback(window_handle, [](GLFWwindow* const window, int key, int action, int scancode, int mods)
	{
		// Prevent "unreferenced formal parameter" warning from triggering
		scancode, mods;

		if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
			glfwSetWindowShouldClose(window, GLFW_TRUE);
	});

	while (!glfwWindowShouldClose(window_handle))
	{
		glfwPollEvents();
	}

    return 0;
}
