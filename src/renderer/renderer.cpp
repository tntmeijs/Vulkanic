//////////////////////////////////////////////////////////////////////////

// Vulkanic
#include "miscellaneous/global_settings.hpp"
#include "renderer.hpp"

//////////////////////////////////////////////////////////////////////////

// C++ standard
#include <algorithm>

//////////////////////////////////////////////////////////////////////////
// Vulkan extension functions
//////////////////////////////////////////////////////////////////////////

VkResult CreateDebugUtilsMessengerEXT(
	VkInstance instance,
	const VkDebugUtilsMessengerCreateInfoEXT* create_info,
	const VkAllocationCallbacks* allocator,
	VkDebugUtilsMessengerEXT* debug_messenger)
{
	auto function = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));

	if (function)
		return function(instance, create_info, allocator, debug_messenger);
	else
		return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void DestroyDebugUtilsMessengerEXT(
	VkInstance instance,
	VkDebugUtilsMessengerEXT debug_messenger,
	const VkAllocationCallbacks* allocator)
{
	auto function = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));

	if (function)
		function(instance, debug_messenger, allocator);
}

//////////////////////////////////////////////////////////////////////////

using namespace vkc;

Renderer::Renderer()
	: m_window(nullptr)
{}

Renderer::~Renderer()
{
#ifdef _DEBUG
	DestroyDebugUtilsMessengerEXT(m_instance, m_debug_messenger, nullptr);
#endif

	vkDestroyInstance(m_instance, nullptr);

	glfwDestroyWindow(m_window);
	glfwTerminate();
}

void Renderer::InitializeVulkan()
{
	CreateInstance();
	SetUpDebugMessenger();
	SelectPhysicalDevice();
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

	// All Vulkan extensions required by the application
	auto required_extensions = GetRequiredExtensions();

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

	// Check whether all required extensions are supported on this system
	for (auto & required_extension : required_extensions)
	{
		bool found_extension = false;

		for (const auto& extension : extensions)
		{
			if (strcmp(extension.extensionName, required_extension) == 0)
				found_extension = true;
		}

		if (!found_extension)
			spdlog::error("\t\tGLFW requires the extension \"{}\" to be present, but it could not be found.\n", required_extension);
	}

	// Use validation layers in debug mode
	bool use_validation_layers = false;
#ifdef _DEBUG
	use_validation_layers = true;
#endif

	VkInstanceCreateInfo instance_info = {};
	instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instance_info.pApplicationInfo = &app_info;
	instance_info.enabledExtensionCount = static_cast<uint32_t>(required_extensions.size());
	instance_info.ppEnabledExtensionNames = required_extensions.data();

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
		instance_info.enabledLayerCount = static_cast<uint32_t>(global_settings::validation_layer_names.size());
		instance_info.ppEnabledLayerNames = global_settings::validation_layer_names.data();
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
	for (const char* name : global_settings::validation_layer_names)
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

std::vector<const char*> Renderer::GetRequiredExtensions()
{
	uint32_t glfw_required_extension_count = 0;
	const char** glfw_extensions;

	glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_required_extension_count);

	std::vector<const char*> required_extensions(glfw_extensions, glfw_extensions + glfw_required_extension_count);

#ifdef _DEBUG
	// When running in debug mode, add the message callback extension to the list
	required_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

	// Add any additional extension names to the list specified in the global settings file
	required_extensions.insert(required_extensions.end(), global_settings::extension_names.begin(), global_settings::extension_names.end());

	// Return the complete list of extensions
	return required_extensions;
}

void Renderer::SetUpDebugMessenger()
{
	// Only set-up the debug messenger in debug builds
#ifdef _DEBUG
	VkDebugUtilsMessengerCreateInfoEXT create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	create_info.messageSeverity =
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	create_info.messageType =
		VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT		|
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT	|
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	create_info.pfnUserCallback = DebugMessageCallback;
#endif

	if (CreateDebugUtilsMessengerEXT(m_instance, &create_info, nullptr, &m_debug_messenger) != VK_SUCCESS)
	{
		spdlog::error("Could not set-up the debug messenger.");
	}
}

void Renderer::SelectPhysicalDevice()
{
	uint32_t physical_device_count = 0;
	vkEnumeratePhysicalDevices(m_instance, &physical_device_count, nullptr);

	if (physical_device_count == 0)
	{
		spdlog::error("No physical device on this computer has Vulkan support.");
		return;
	}

	std::vector<VkPhysicalDevice> physical_devices(physical_device_count);
	vkEnumeratePhysicalDevices(m_instance, &physical_device_count, physical_devices.data());

	std::vector<std::pair<uint32_t, VkPhysicalDevice>> physical_device_scores;
	physical_device_scores.reserve(physical_device_count);

	// Rate the devices based on their capabilities
	for (const auto& physical_device : physical_devices)
	{
		physical_device_scores.emplace_back(RatePhysicalDeviceSuitability(physical_device), physical_device);
	}

	// Sort the vector in descending order, the first element will be the best GPU available
	std::sort(physical_device_scores.begin(), physical_device_scores.end(), [](
		const std::pair<uint32_t, VkPhysicalDevice>& a,
		const std::pair<uint32_t, VkPhysicalDevice>& b)
	{
		return (a.first > b.first);
	});

	// Now that the GPUs have been sorted, it is safe to assume that the first element is the best GPU available
	// However, if the score is zero, it means the GPU lacks certain required features
	if (physical_device_scores[0].first == 0)
	{
		spdlog::error("Could not find a suitable GPU.");
		return;
	}

	m_physical_device = physical_device_scores[0].second;
}

uint32_t Renderer::RatePhysicalDeviceSuitability(const VkPhysicalDevice& physical_device)
{
	uint32_t score = 0;

	VkPhysicalDeviceProperties properties	= {};
	VkPhysicalDeviceFeatures features		= {};

	vkGetPhysicalDeviceProperties(physical_device, &properties);
	vkGetPhysicalDeviceFeatures(physical_device, &features);

	// A discrete GPU is always preferred, hence the big increase in score
	if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		score += 1000;

	// #TODO: Add any additional checks (e.g. VK_NV_ray_tracing support, maximum texture size, etc.)
	
	// #TODO: If a REQUIRED feature is not present, return 0 (a score of 0 will not be accepted by the application)

	return score;
}

VKAPI_ATTR VkBool32 VKAPI_CALL Renderer::DebugMessageCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT severity,
	VkDebugUtilsMessageTypeFlagsEXT type,
	const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
	void* user_data)
{
	// Suppress "unreferenced formal parameter" warning when using warning level 4
	type;
	user_data;

	// Only log warnings to the console
	if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
		spdlog::warn("Validation layer: {}", callback_data->pMessage);

	return VK_FALSE;
}
