//////////////////////////////////////////////////////////////////////////

// Renderer
#include "renderer.hpp"

//////////////////////////////////////////////////////////////////////////

using namespace vkc;

Renderer::Renderer()
	: m_window(nullptr)
{}

Renderer::~Renderer()
{
	glfwDestroyWindow(m_window);
	glfwTerminate();
}

void Renderer::InitializeVulkan()
{}

void Renderer::SetupWindow(uint32_t width, uint32_t height, const char* title)
{
	if (!glfwInit())
	{
		spdlog::error("Could not initialize GLFW.");
		return;
	}

	spdlog::info("GLFW has been initialized.");

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	m_window = glfwCreateWindow(width, height, title, nullptr, nullptr);

	if (!m_window)
	{
		glfwDestroyWindow(m_window);
		glfwTerminate();
		spdlog::error("Could not create a window.");
		return;
	}

	glfwMakeContextCurrent(m_window);

	spdlog::info("A window has been created.");
}

void Renderer::Prepare()
{}

void Renderer::Run()
{}

GLFWwindow* const Renderer::GetHandle() const
{
	return m_window;
}
