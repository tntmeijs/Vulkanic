//////////////////////////////////////////////////////////////////////////

// Renderer
#include "renderer/renderer.hpp"

//////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[])
{
	// Prevent "unreferenced formal parameter" warning from triggering
	argc, argv;

	vkc::Renderer renderer;
	renderer.SetupWindow();
	renderer.InitializeVulkan();

	auto* const window_handle = renderer.GetHandle();
	
	glfwSetKeyCallback(window_handle, [](GLFWwindow* const window, int key, int action, int scancode, int mods)
	{
		// Prevent "unreferenced formal parameter" warning from triggering
		scancode, mods;

		if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
			glfwSetWindowShouldClose(window, GLFW_TRUE);
	});

	glfwSetFramebufferSizeCallback(window_handle, [](GLFWwindow* const window, int width, int height)
	{
		// Prevent "unreferenced formal parameter" warning from triggering
		width, height;

		auto* const renderer = reinterpret_cast<vkc::Renderer*>(glfwGetWindowUserPointer(window));
		renderer->TriggerFramebufferResized();
	});

	while (!glfwWindowShouldClose(window_handle))
	{
		glfwPollEvents();
		renderer.Update();
		renderer.Draw();
	}

    return 0;
}
