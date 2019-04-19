#include <spdlog/spdlog.h>
#include <vulkan/vulkan.hpp>
#include <glfw/glfw3.h>

int main(int argc, char* argv[])
{
	if (!glfwInit())
	{
		spdlog::error("Could not initialize GLFW.");
		return -1;
	}

	spdlog::info("GLFW has been initialized.");

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	auto* const window = glfwCreateWindow(1280, 720, "Vulkanic", nullptr, nullptr);

	if (!window)
	{
		glfwDestroyWindow(window);
		glfwTerminate();
		spdlog::error("Could not create a window.");
		return -1;
	}

	glfwMakeContextCurrent(window);

	glfwSetKeyCallback(window, [](GLFWwindow* const window, int key, int action, int scancode, int mods) {
		if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
			glfwSetWindowShouldClose(window, GLFW_TRUE);
	});

	spdlog::info("A window has been created.");

	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
	}

	glfwDestroyWindow(window);
	glfwTerminate();

    return 0;
}
