//////////////////////////////////////////////////////////////////////////

// Vulkanic
#include "miscellaneous/global_settings.hpp"
#include "renderer.hpp"

//////////////////////////////////////////////////////////////////////////

// C++ standard
#include <algorithm>
#include <set>
#include <string>

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
	for (const auto& image_view : m_swapchain_image_views)
		vkDestroyImageView(m_device, image_view, nullptr);

	vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
	vkDestroyDevice(m_device, nullptr);

#ifdef _DEBUG
	DestroyDebugUtilsMessengerEXT(m_instance, m_debug_messenger, nullptr);
#endif

	vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
	vkDestroyInstance(m_instance, nullptr);

	glfwDestroyWindow(m_window);
	glfwTerminate();
}

void Renderer::InitializeVulkan()
{
	CreateInstance();
	SetUpDebugMessenger();
	CreateSurface();
	SelectPhysicalDevice();
	CreateLogicalDevice();
	CreateSwapchain();
	CreateSwapchainImageViews();
}

void Renderer::SetupWindow()
{
	if (!glfwInit())
	{
		spdlog::error("Could not initialize GLFW.");
		assert(false);
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
		assert(false);
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
	auto required_extensions = GetRequiredInstanceExtensions();

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
	for (auto& required_extension : required_extensions)
	{
		bool found_extension = false;

		for (const auto& extension : extensions)
		{
			if (strcmp(extension.extensionName, required_extension) == 0)
				found_extension = true;
		}

		if (!found_extension)
		{
			spdlog::error("\t\tGLFW requires the extension \"{}\" to be present, but it could not be found.\n", required_extension);
			assert(false);
		}
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

std::vector<const char*> Renderer::GetRequiredInstanceExtensions()
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
	required_extensions.insert(required_extensions.end(), global_settings::instance_extension_names.begin(), global_settings::instance_extension_names.end());

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

void Renderer::CreateSurface()
{
	if (glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_surface) != VK_SUCCESS)
	{
		spdlog::error("Could not create a window surface.");
	}
}

void Renderer::SelectPhysicalDevice()
{
	uint32_t physical_device_count = 0;
	vkEnumeratePhysicalDevices(m_instance, &physical_device_count, nullptr);

	if (physical_device_count == 0)
	{
		spdlog::error("No physical device on this computer has Vulkan support.");
		assert(false);
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
	bool found_gpu_with_valid_score = false;
	for (const auto& pair : physical_device_scores)
	{
		// Grab the first GPU with a valid score (scores are sorted already, so this should be the best possible GPU)
		if (pair.first != 0)
		{
			found_gpu_with_valid_score = true;
			m_physical_device = pair.second;
		}
	}

	if (!found_gpu_with_valid_score)
	{
		spdlog::error("Could not find a suitable GPU on this system.");
		assert(false);
	}

	// Try to find all required queue family indices
	m_queue_family_indices = FindQueueFamiliesOfSelectedPhysicalDevice();

	// If the device does not support all required queue families, it is not usable at all for this application
	if (!m_queue_family_indices.AllIndicesFound())
	{
		spdlog::error("Not all required queue family indices could be found on this system.");
		assert(false);
	}

	// Check whether all required device extensions are supported
	if (!CheckPhysicalDeviceExtensionSupport())
	{
		spdlog::error("Not all required device extensions are available on this system.");
		assert(false);
	}

	// Find out what kind of swapchain can be created
	QuerySwapchainSupport();

	// The swapchain is good enough if we have at least one presentation mode, and a supported image format
	if (m_swap_chain_support.formats.empty() ||
		m_swap_chain_support.present_modes.empty())
	{
		spdlog::error("Swapchain support is not adequate.");
		assert(false);
	}

	VkPhysicalDeviceProperties gpu_properties;
	VkPhysicalDeviceMemoryProperties gpu_memory_properties;
	vkGetPhysicalDeviceProperties(m_physical_device, &gpu_properties);
	vkGetPhysicalDeviceMemoryProperties(m_physical_device, &gpu_memory_properties);

	// List the GPU information
	spdlog::info("Selected GPU: \"{}\".", gpu_properties.deviceName);
	for (uint32_t i = 0; i < gpu_memory_properties.memoryHeapCount; ++i)
	{
		// Find the heap that represents the VRAM
		if (gpu_memory_properties.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
			spdlog::info("  > VRAM:\t{}MB", (gpu_memory_properties.memoryHeaps[i].size / 1024 / 1024));
		else
			spdlog::info("  > Shared:\t{}MB", (gpu_memory_properties.memoryHeaps[i].size / 1024 / 1024));
	}
}

uint32_t Renderer::RatePhysicalDeviceSuitability(const VkPhysicalDevice& physical_device)
{
	uint32_t score = 0;

	VkPhysicalDeviceProperties properties			= {};
	VkPhysicalDeviceFeatures features				= {};
	VkPhysicalDeviceMemoryProperties mem_properties	= {};

	vkGetPhysicalDeviceProperties(physical_device, &properties);
	vkGetPhysicalDeviceFeatures(physical_device, &features);
	vkGetPhysicalDeviceMemoryProperties(physical_device, &mem_properties);

	// A discrete GPU is always preferred, hence the big increase in score
	if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		score += 1000;

	// More VRAM is better
	for (uint32_t i = 0; i < mem_properties.memoryHeapCount; ++i)
	{
		// Find the heap that represents the VRAM
		if (mem_properties.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
			score += static_cast<uint32_t>(mem_properties.memoryHeaps[i].size / 1024 / 1024);
	}

	// #TODO: Add any additional checks (e.g. VK_NV_ray_tracing support, maximum texture size, etc.)
	
	// #TODO: If a REQUIRED feature is not present, return 0 (a score of 0 will not be accepted by the application)

	return score;
}

QueueFamilyIndices Renderer::FindQueueFamiliesOfSelectedPhysicalDevice()
{
	QueueFamilyIndices indices;

	uint32_t queue_family_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(m_physical_device, &queue_family_count, nullptr);

	std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
	vkGetPhysicalDeviceQueueFamilyProperties(m_physical_device, &queue_family_count, queue_families.data());

	if (queue_family_count == 0)
		spdlog::error("Could not find any queue families using this physical device.");

	uint32_t index = 0;
	for (const auto& queue_family : queue_families)
	{
		// Does this queue family support presenting?
		VkBool32 present_supported = true;
		vkGetPhysicalDeviceSurfaceSupportKHR(m_physical_device, index, m_surface, &present_supported);

		// Look for a queue family that supports present operations
		if (present_supported)
			indices.present_family_index = index;

		// Look for a queue family that supports graphics operations
		if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			indices.graphics_family_index = index;

		// Stop searching once all queue family indices have been found
		if (indices.AllIndicesFound())
			break;

		++index;
	}

	return indices;
}

void Renderer::CreateLogicalDevice()
{
	float queue_priority = 1.0f;

	// Eliminate duplicate family indices by using the set (guarantees unique values)
	std::set<uint32_t> unique_family_indices =
	{
		m_queue_family_indices.graphics_family_index.value(),
		m_queue_family_indices.present_family_index.value()
	};
	
	std::vector<VkDeviceQueueCreateInfo> queue_create_infos;

	for (const auto& unique_index : unique_family_indices)
	{
		VkDeviceQueueCreateInfo queue_create_info = {};
		queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queue_create_info.queueFamilyIndex = unique_index;
		queue_create_info.queueCount = 1;
		queue_create_info.pQueuePriorities = &queue_priority;

		queue_create_infos.push_back(queue_create_info);
	}

	// #TODO: Add device features here once the application needs it
	VkPhysicalDeviceFeatures device_features = {};

	VkDeviceCreateInfo device_create_info = {};
	device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	device_create_info.pQueueCreateInfos = queue_create_infos.data();
	device_create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
	device_create_info.pEnabledFeatures = &device_features;
	device_create_info.enabledExtensionCount = static_cast<uint32_t>(global_settings::device_extension_names.size());
	device_create_info.ppEnabledExtensionNames = global_settings::device_extension_names.data();
	
	if (vkCreateDevice(m_physical_device, &device_create_info, nullptr, &m_device) != VK_SUCCESS)
		spdlog::error("Could not create a logical device.");

	spdlog::info("Successfully created a logical device.");

	// Queues are created as soon as the logical device is created, which means handles to the queues can be retrieved
	vkGetDeviceQueue(m_device, m_queue_family_indices.graphics_family_index.value(), 0, &m_graphics_queue);
	vkGetDeviceQueue(m_device, m_queue_family_indices.present_family_index.value(), 0, &m_present_queue);
}

bool Renderer::CheckPhysicalDeviceExtensionSupport()
{
	uint32_t extension_count = 0;
	vkEnumerateDeviceExtensionProperties(m_physical_device, nullptr, &extension_count, nullptr);

	if (extension_count == 0)
	{
		spdlog::error("Could not find any extensions on this system.");
		return false;
	}

	std::vector<VkExtensionProperties> all_extensions(extension_count);
	vkEnumerateDeviceExtensionProperties(m_physical_device, nullptr, &extension_count, all_extensions.data());

	for (const auto& required_extension : global_settings::device_extension_names)
	{
		bool extension_found = false;

		for (const auto& extension : all_extensions)
		{
			if (strcmp(required_extension, extension.extensionName) == 0)
			{
				extension_found = true;
				break;
			}
		}

		if (!extension_found)
		{
			spdlog::error("The extension \"{}\" is not available, but it is required by the application.", required_extension);
			return false;
		}
	}

	return true;
}

void Renderer::QuerySwapchainSupport()
{
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physical_device, m_surface, &m_swap_chain_support.capabilities);

	uint32_t format_count = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(m_physical_device, m_surface, &format_count, nullptr);

	if (format_count == 0)
	{
		spdlog::error("No valid swapchain formats could be found.");
		assert(false);
	}

	m_swap_chain_support.formats.resize(format_count);
	vkGetPhysicalDeviceSurfaceFormatsKHR(m_physical_device, m_surface, &format_count, m_swap_chain_support.formats.data());

	uint32_t present_mode_count = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(m_physical_device, m_surface, &present_mode_count, nullptr);
	
	if (present_mode_count == 0)
	{
		spdlog::error("No valid swapchain present modes could be found.");
		assert(false);
	}

	m_swap_chain_support.present_modes.resize(present_mode_count);
	vkGetPhysicalDeviceSurfacePresentModesKHR(m_physical_device, m_surface, &present_mode_count, m_swap_chain_support.present_modes.data());
}

VkSurfaceFormatKHR Renderer::ChooseSwapchainSurfaceFormat()
{
	// Vulkan has no preferred format, use our own preferred format
	if (m_swap_chain_support.formats.size() == 1 && m_swap_chain_support.formats[0].format == VK_FORMAT_UNDEFINED)
	{
		return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
	}

	// Find the most optimal format out of all the preferred formats returned by this system
	for (const auto& format : m_swap_chain_support.formats)
	{
		// Check if our preferred format is in the list of formats
		if (format.format == VK_FORMAT_B8G8R8A8_UNORM && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			return format;
	}

	// Our preferred format is not in the list, just use the first format, then...
	return m_swap_chain_support.formats[0];
}

VkPresentModeKHR Renderer::ChooseSwapchainSurfacePresentMode()
{
	for (const auto& present_mode : m_swap_chain_support.present_modes)
	{
		// Ideally, the application can use the mailbox present mode
		if (present_mode == VK_PRESENT_MODE_MAILBOX_KHR)
			return present_mode;
		// Less ideal, but still OK
		else if (present_mode == VK_PRESENT_MODE_IMMEDIATE_KHR)
			return present_mode;
	}

	// Just use the present mode that is guaranteed to be available
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Renderer::ChooseSwapchainExtent()
{
	const auto& capabilities = m_swap_chain_support.capabilities;

	// Use the default extent
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
		return capabilities.currentExtent;
	
	// Use the resolution that matches the window within the given extents the best
	VkExtent2D extent = { global_settings::default_window_width, global_settings::default_window_height };

	extent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, extent.width));
	extent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, extent.height));

	return extent;
}

void Renderer::CreateSwapchain()
{
	auto surface_format			= ChooseSwapchainSurfaceFormat();
	auto surface_extent			= ChooseSwapchainExtent();
	auto surface_present_mode	= ChooseSwapchainSurfacePresentMode();

	// Make sure we have at least one more image than the recommended amount (to avoid having to wait on internal operations)
	uint32_t image_count = m_swap_chain_support.capabilities.minImageCount + 1;

	// Do not exceed the maximum allowed swapchain image count
	if (m_swap_chain_support.capabilities.maxImageCount > 0 &&
		image_count > m_swap_chain_support.capabilities.maxImageCount)
	{
		image_count = m_swap_chain_support.capabilities.maxImageCount;
	}

	// Queue families grouped in an array for easy access when filling out the swapchain create structure
	uint32_t queue_families[] =
	{
		m_queue_family_indices.graphics_family_index.value(),
		m_queue_family_indices.present_family_index.value()
	};

	VkSwapchainCreateInfoKHR create_info = {};
	create_info.sType					= VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	create_info.surface					= m_surface;
	create_info.minImageCount			= image_count;
	create_info.imageFormat				= surface_format.format;
	create_info.imageColorSpace			= surface_format.colorSpace;
	create_info.imageExtent				= surface_extent;
	create_info.imageArrayLayers		= 1;
	create_info.imageUsage				= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	create_info.preTransform			= m_swap_chain_support.capabilities.currentTransform;
	create_info.compositeAlpha			= VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	create_info.presentMode				= surface_present_mode;
	create_info.clipped					= VK_TRUE;
	create_info.oldSwapchain			= VK_NULL_HANDLE;

	if (m_queue_family_indices.graphics_family_index.value() != m_queue_family_indices.present_family_index.value())
	{
		create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		create_info.queueFamilyIndexCount = sizeof(queue_families) / sizeof(uint32_t);
		create_info.pQueueFamilyIndices = queue_families;
	}
	else
	{
		create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	if (vkCreateSwapchainKHR(m_device, &create_info, nullptr, &m_swapchain) != VK_SUCCESS)
	{
		spdlog::error("Could not create a swapchain.");
	}

	spdlog::info("Successfully created a swapchain.");

	// Get handles to the images in the swapchain
	vkGetSwapchainImagesKHR(m_device, m_swapchain, &image_count, nullptr);
	m_swapchain_images.resize(image_count);
	vkGetSwapchainImagesKHR(m_device, m_swapchain, &image_count, m_swapchain_images.data());

	// Store the format and extent of the swapchain for future use
	m_swapchain_format = surface_format.format;
	m_swapchain_extent = surface_extent;
}

void Renderer::CreateSwapchainImageViews()
{
	m_swapchain_image_views.resize(m_swapchain_images.size());

	uint32_t index = 0;
	for (const auto& image : m_swapchain_images)
	{
		VkImageViewCreateInfo image_view_create_info = {};
		image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		image_view_create_info.image = image;
		image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		image_view_create_info.format = m_swapchain_format;
		image_view_create_info.components =
		{
			VK_COMPONENT_SWIZZLE_IDENTITY,	// R
			VK_COMPONENT_SWIZZLE_IDENTITY,	// G
			VK_COMPONENT_SWIZZLE_IDENTITY,	// B
			VK_COMPONENT_SWIZZLE_IDENTITY	// A
		};
		image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		image_view_create_info.subresourceRange.baseMipLevel = 0;
		image_view_create_info.subresourceRange.levelCount = 1;
		image_view_create_info.subresourceRange.baseArrayLayer = 0;
		image_view_create_info.subresourceRange.layerCount = 1;

		if (vkCreateImageView(m_device, &image_view_create_info, nullptr, &m_swapchain_image_views[index]) != VK_SUCCESS)
		{
			spdlog::error("Could not create an image view for the swapchain image.");
			assert(false);
		}

		++index;
	}

	spdlog::info("Successfully created image views for all swapchain images.");
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
