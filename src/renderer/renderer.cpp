//////////////////////////////////////////////////////////////////////////

// Vulkanic
#include "miscellaneous/global_settings.hpp"
#include "renderer.hpp"

//////////////////////////////////////////////////////////////////////////

// C++ standard
#include <vector>

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

void Renderer::SetupWindow()
{
	if (!glfwInit())
	{
		spdlog::error("Could not initialize GLFW.");
		return;
	}

	spdlog::info("GLFW has been initialized.");

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	m_window = glfwCreateWindow(
		global_settings::default_window_width,
		global_settings::default_window_height,
		global_settings::window_title,
		nullptr, nullptr);

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
	app_info.pApplicationName = global_settings::application_name;
	app_info.applicationVersion = VK_MAKE_VERSION(
		global_settings::application_version[0],
		global_settings::application_version[1],
		global_settings::application_version[2]);
	app_info.pEngineName = global_settings::engine_name;
	app_info.engineVersion = VK_MAKE_VERSION(
		global_settings::engine_version[0],
		global_settings::engine_version[1],
		global_settings::engine_version[2]);
	app_info.apiVersion = VK_API_VERSION_1_0;

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
	spdlog::info("Found the following available extensions:");
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

	// Use validation layers in debug mode
	bool use_validation_layers = false;
#ifdef _DEBUG
	use_validation_layers = true;
#endif

	VkInstanceCreateInfo instance_info = {};
	instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instance_info.pApplicationInfo = &app_info;
	instance_info.enabledExtensionCount = glfw_extension_count;
	instance_info.ppEnabledExtensionNames = glfw_extensions;

	if (!CheckValidationLayerSupport() || !use_validation_layers)
	{
		// Display the proper console message
		if (!use_validation_layers)
			spdlog::info("Validation layers have been disabled.");
		else
			spdlog::error("One or more validation layers could not be found, no validation layers will be used.");

		instance_info.enabledLayerCount = 0;
	}
	else
	{
		instance_info.enabledLayerCount = static_cast<uint32_t>(global_settings::validation_layers.size());
		instance_info.ppEnabledLayerNames = global_settings::validation_layers.data();
	}

	auto result = vkCreateInstance(&instance_info, nullptr, &m_instance);
	if (result != VK_SUCCESS)
		spdlog::error("Could not create a Vulkan instance.");
	else
		spdlog::info("Vulkan instance created successfully.");
}

bool vkc::Renderer::CheckValidationLayerSupport()
{
	uint32_t validation_layer_count = 0;
	vkEnumerateInstanceLayerProperties(&validation_layer_count, nullptr);

	std::vector<VkLayerProperties> available_validation_layers(validation_layer_count);
	vkEnumerateInstanceLayerProperties(&validation_layer_count, available_validation_layers.data());

	spdlog::info("Found the following available validation layers:");
	for (const auto& layer_properties : available_validation_layers)
	{
		spdlog::info("  > {}", layer_properties.layerName);
	}

	// Check if the requested validation layers are supported
	for (const char* name : global_settings::validation_layers)
	{
		bool layer_found = false;

		for (const auto& layer_properties : available_validation_layers)
		{
			if (strcmp(name, layer_properties.layerName) == 0)
			{
				// Found a requested layer in the list of supported layers
				layer_found = true;
				break;
			}
		}

		// Could not find the requested layer in the list of supported layers
		if (!layer_found)
			return false;
	}

	// Requested layers are all supported
	return true;
}
