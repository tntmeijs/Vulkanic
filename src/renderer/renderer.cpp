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
	vkDestroyInstance(m_instance, nullptr);

	glfwDestroyWindow(m_window);
	glfwTerminate();
}

void Renderer::InitializeVulkan()
{
	CreateInstance();
}

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

GLFWwindow* const Renderer::GetHandle() const
{
	return m_window;
}

void Renderer::CreateInstance()
{
	VkApplicationInfo app_info = {};
	app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app_info.pApplicationName = "Vulkanic";
	app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.pEngineName = "Vulkanic";
	app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.apiVersion = VK_VERSION_1_0;

	// All Vulkan extensions required by GLFW
	uint32_t glfw_extension_count = 0;
	const char** glfw_extensions;
	glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

	// Retrieve a list of all supported extensions
	uint32_t extension_count = 0;
	std::vector<VkExtensionProperties> extensions;
	vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);
	extensions.resize(extension_count);
	vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, extensions.data());

	// Log all available extensions
	spdlog::info("Available Vulkan extensions:");
	for (const auto& extension : extensions)
	{
		spdlog::info("  > {}", extension.extensionName);
	}

	// Check whether all GLFW required extensions are supported on this system
	for (uint32_t i = 0; i < glfw_extension_count; ++i)
	{
		bool found_extension = false;

		for (const auto& extension : extensions)
		{
			if (strcmp(extension.extensionName, glfw_extensions[i]) == 0)
				found_extension = true;
		}

		if (!found_extension)
			spdlog::error("\t\tGLFW requires the extension \"{}\" to be present, but it could not be found.\n", glfw_extensions[i]);
	}

	VkInstanceCreateInfo instance_info = {};
	instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instance_info.pApplicationInfo = &app_info;
	instance_info.enabledExtensionCount = glfw_extension_count;
	instance_info.ppEnabledExtensionNames = glfw_extensions;
	instance_info.enabledLayerCount = 0;

	auto result = vkCreateInstance(&instance_info, nullptr, &m_instance);
	if (result != VK_SUCCESS)
		spdlog::error("Could not create a Vulkan instance.");
	else
		spdlog::info("Vulkan instance created successfully.");
}
