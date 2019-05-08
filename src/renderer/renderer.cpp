//////////////////////////////////////////////////////////////////////////

// Vulkanic
#include "miscellaneous/global_settings.hpp"
#include "renderer.hpp"

//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////

// GLM
#include "glm/gtc/matrix_transform.hpp"
#include "glm/mat4x4.hpp"
#include "glm/vec3.hpp"

//////////////////////////////////////////////////////////////////////////

// C++ standard
#include <algorithm>
#include <array>
#include <fstream>
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

// Hard-coded models
struct Vertex
{
	glm::vec3 position;
	glm::vec3 color;

	static VkVertexInputBindingDescription GetBindingDescription()
	{
		VkVertexInputBindingDescription desc = {};
		desc.binding = 0;
		desc.stride = sizeof(Vertex);
		desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return desc;
	}

	static std::array<VkVertexInputAttributeDescription, 2> GetAttributeDescriptions()
	{
		std::array<VkVertexInputAttributeDescription, 2> attribs;

		attribs[0].binding = 0;
		attribs[0].location = 0;
		attribs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attribs[0].offset = offsetof(Vertex, position);

		attribs[1].binding = 0;
		attribs[1].location = 1;
		attribs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attribs[1].offset = offsetof(Vertex, color);

		return attribs;
	}
};

const std::vector<Vertex> vertices =
{
	// Position					// Color
	{ {  0.0f, -0.5f, 0.0f }, { 1.0f, 1.0f, 1.0f } },
	{ { -0.5f,  0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f } },
	{ {  0.5f,  0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f } }
};

struct CameraData
{
	glm::mat4 model_matrix;
	glm::mat4 view_matrix;
	glm::mat4 projection_matrix;
};

Renderer::Renderer()
	: m_window(nullptr)
	, m_frame_index(0)
	, m_framebuffer_resized(false)
{}

Renderer::~Renderer()
{
	// Wait until the GPU finishes the current operation before cleaning-up resources
	vkDeviceWaitIdle(m_device);

	CleanUpSwapchain();

	vkDestroyDescriptorSetLayout(m_device, m_camera_data_descriptor_set_layout, nullptr);
	vkDestroyBuffer(m_device, m_vertex_buffer, nullptr);
	vkFreeMemory(m_device, m_vertex_buffer_memory, nullptr);

	for (auto index = 0; index < global_settings::maximum_in_flight_frame_count; ++index)
	{
		vkDestroySemaphore(m_device, m_in_flight_frame_image_available_semaphores[index], nullptr);
		vkDestroySemaphore(m_device, m_in_flight_render_finished_semaphores[index], nullptr);
	}

	for (auto index = 0; index < global_settings::maximum_in_flight_frame_count; ++index)
	{
		vkDestroyFence(m_device, m_in_flight_fences[index], nullptr);
	}
	
	vkFreeCommandBuffers(m_device, m_transfer_command_pool, 1, &m_transfer_command_buffer);
	vkDestroyCommandPool(m_device, m_transfer_command_pool, nullptr);
	vkDestroyCommandPool(m_device, m_graphics_command_pool, nullptr);
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
	CreateRenderPass();
	CreateDescriptorSetLayout();
	CreateGraphicsPipeline();
	CreateFramebuffers();
	CreateCommandPools();
	CreateTransferCommandBuffer();
	CreateVertexBuffer();
	CreateUniformBuffers();
	CreateDescriptorPool();
	CreateDescriptorSets();
	CreateCommandBuffers();
	CreateSynchronizationObjects();
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

	m_window = glfwCreateWindow(
		global_settings::default_window_width,
		global_settings::default_window_height,
		global_settings::window_title,
		nullptr, nullptr);

	// Save the "this" pointer for use in the callbacks
	glfwSetWindowUserPointer(m_window, this);

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

void Renderer::Draw()
{
	// Wait for the fence of the old frame to be completed
	vkWaitForFences(m_device, 1, &m_in_flight_fences[m_frame_index], VK_TRUE, std::numeric_limits<uint64_t>::max());

	// The index stores the index of the swapchain image that is available for writing
	uint32_t current_swapchain_image_index = 0;

	// Retrieve an image from the swapchain for writing (wait indefinitely for the image to become available)
	auto result = vkAcquireNextImageKHR(m_device, m_swapchain, std::numeric_limits<uint64_t>::max(), m_in_flight_frame_image_available_semaphores[m_frame_index], VK_NULL_HANDLE, &current_swapchain_image_index);

	// Recreate the swapchain if the current swapchain has become incompatible with the surface
	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		RecreateSwapchain();
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		spdlog::error("Could not acquire a new swapchain image.");
	}

	// Wait on these semaphores before execution can start
	VkSemaphore wait_semaphores[] = { m_in_flight_frame_image_available_semaphores[m_frame_index] };

	// Signal these semaphores once execution finishes
	VkSemaphore signal_semaphores[] = { m_in_flight_render_finished_semaphores[m_frame_index] };

	// Wait in these stages of the pipeline on the semaphores
	VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

	VkSubmitInfo submit_info = {};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.waitSemaphoreCount = sizeof(wait_semaphores) / sizeof(wait_semaphores[0]);
	submit_info.pWaitSemaphores = wait_semaphores;
	submit_info.pWaitDstStageMask = wait_stages;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &m_command_buffers[current_swapchain_image_index];
	submit_info.signalSemaphoreCount = sizeof(signal_semaphores) / sizeof(signal_semaphores[0]);
	submit_info.pSignalSemaphores = signal_semaphores;

	// Fence completed, reset its state
	vkResetFences(m_device, 1, &m_in_flight_fences[m_frame_index]);

	// Submit the command queue
	if (vkQueueSubmit(m_graphics_queue, 1, &submit_info, m_in_flight_fences[m_frame_index]) != VK_SUCCESS)
	{
		spdlog::error("Could not submit the queue for frame #{}.", current_swapchain_image_index);
		return;
	}

	VkPresentInfoKHR present_info = {};
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.waitSemaphoreCount = sizeof(signal_semaphores) / sizeof(signal_semaphores[0]);
	present_info.pWaitSemaphores = signal_semaphores;
	present_info.swapchainCount = 1;
	present_info.pSwapchains = &m_swapchain;
	present_info.pImageIndices = &current_swapchain_image_index;

	// Request to present an image to the swapchain
	result = vkQueuePresentKHR(m_present_queue, &present_info);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_framebuffer_resized)
	{
		spdlog::warn("Swapchain is not up-to-date anymore, recreating swapchain...");

		m_framebuffer_resized = false;
		RecreateSwapchain();
	}
	else if (result != VK_SUCCESS)
	{
		spdlog::error("Could not present the swapchain image.");
		return;
	}

	// Advance to the next frame
	m_frame_index = (m_frame_index + 1) % global_settings::maximum_in_flight_frame_count;
}

void Renderer::Update()
{
	static float rotate_amount = 0.0f;
	rotate_amount += 0.01f;

	CameraData cam_data = {};
	cam_data.model_matrix = glm::rotate(glm::mat4(1.0f), rotate_amount * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	cam_data.view_matrix = glm::lookAt(glm::vec3(0.0f, 2.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	cam_data.projection_matrix = glm::perspective(
		90.0f,
		static_cast<float>(m_swapchain_extent.width) / static_cast<float>(m_swapchain_extent.height),
		0.1f,
		1000.0f);

	// GLM is for OpenGL, Vulkan uses an inverted Y-coordinate
	// ubo.projection_matrix[1][1] *= -1.0f;

	void* data = nullptr;
	vkMapMemory(m_device, m_camera_ubos_memory[m_frame_index], 0, sizeof(cam_data), 0, &data);
	memcpy(data, &cam_data, sizeof(cam_data));
	vkUnmapMemory(m_device, m_camera_ubos_memory[m_frame_index]);
}

void Renderer::TriggerFramebufferResized()
{
	m_framebuffer_resized = true;
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
	spdlog::info("Found the following available instance extensions:");
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
		// Look for a dedicated transfer queue family
		if ((queue_family.queueFlags & VK_QUEUE_TRANSFER_BIT)		&&
			(queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0	&&
			(queue_family.queueFlags & VK_QUEUE_COMPUTE_BIT) == 0)
		{
			indices.transfer_family_index = { 0, 0 };
			indices.transfer_family_index->first = index;
			indices.transfer_family_index->second = queue_family.queueCount;
			continue;
		}

		// Does this queue family support presenting?
		VkBool32 present_supported = true;
		vkGetPhysicalDeviceSurfaceSupportKHR(m_physical_device, index, m_surface, &present_supported);

		// Look for a queue family that supports present operations
		if (present_supported)
		{
			indices.present_family_index = { 0, 0 };
			indices.present_family_index->first = index;
			indices.present_family_index->second = queue_family.queueCount;
		}

		// Look for a queue family that supports graphics operations
		if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			indices.graphics_family_index = { 0, 0 };
			indices.graphics_family_index->first = index;
			indices.graphics_family_index->second = queue_family.queueCount;
		}

		// Stop searching once all queue family indices have been found
		if (indices.AllIndicesFound())
			break;

		++index;
	}
	
	return indices;
}

void Renderer::CreateLogicalDevice()
{
	// Eliminate duplicate family indices by using the set (guarantees unique values)
	std::set<uint32_t> unique_family_indices =
	{
		m_queue_family_indices.graphics_family_index->first,
		m_queue_family_indices.present_family_index->first
	};
	
	std::vector<VkDeviceQueueCreateInfo> queue_create_infos;

	for (const auto& unique_index : unique_family_indices)
	{
		VkDeviceQueueCreateInfo queue_create_info = {};
		queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queue_create_info.queueFamilyIndex = unique_index;

		// If there is no dedicated transfer queue and the graphics family has
		// more than one queue, use two queues from the graphics family instead
		if (m_queue_family_indices.graphics_family_index.value().second >= 2 &&
			unique_index == m_queue_family_indices.graphics_family_index->first)
		{
			float queue_priorities[2] = { 1.0f, 1.0f };

			// A graphics and a transfer queue
			queue_create_info.queueCount = 2;
			queue_create_info.pQueuePriorities = queue_priorities;
		}
		else
		{
			float queue_priority = 1.0f;

			// Just a single queue
			queue_create_info.queueCount = 1;
			queue_create_info.pQueuePriorities = &queue_priority;
		}

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

	vkGetDeviceQueue(m_device, m_queue_family_indices.graphics_family_index->first, 0, &m_graphics_queue);
	vkGetDeviceQueue(m_device, m_queue_family_indices.present_family_index->first, 0, &m_present_queue);
	
	if (!m_queue_family_indices.transfer_family_index.has_value() &&
		m_queue_family_indices.graphics_family_index.value().second >= 2)
	{
		// If a there is no dedicated transfer queue, use a second graphics queue instead
		vkGetDeviceQueue(m_device, m_queue_family_indices.graphics_family_index->first, 1, &m_transfer_queue);

		spdlog::warn("Could not find a dedicated transfer queue, using a second graphics queue instead...");
	}
	else if (m_queue_family_indices.graphics_family_index.value().second == 1)
	{
		// Only one queue in the graphics family is available, use the graphics queue as a transfer queue
		vkGetDeviceQueue(m_device, m_queue_family_indices.graphics_family_index->first, 0, &m_transfer_queue);
	
		spdlog::warn("Could not find a dedicated transfer queue or a second graphics queue, using the main graphics queue instead...");
	}
	else
	{
		// A dedicated transfer queue is available
		vkGetDeviceQueue(m_device, m_queue_family_indices.transfer_family_index->first, 0, &m_transfer_queue);
	}
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

	// Log all available device extensions
	spdlog::info("Found the following available device extensions:");
	for (const auto& extension : all_extensions)
	{
		spdlog::info("  > {}", extension.extensionName);
	}

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
	int width, height;
	glfwGetFramebufferSize(m_window, &width, &height);
	VkExtent2D extent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };

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
		m_queue_family_indices.graphics_family_index->first,
		m_queue_family_indices.present_family_index->first
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

void Renderer::CreateGraphicsPipeline()
{
	auto vertex_shader_code = ReadSPRIVFromFile("./resources/shaders/basic.vert.spv");
	auto fragment_shader_code = ReadSPRIVFromFile("./resources/shaders/basic.frag.spv");

	auto vertex_shader_module = CreateShaderModule(vertex_shader_code);
	auto fragment_shader_module = CreateShaderModule(fragment_shader_code);

	VkPipelineShaderStageCreateInfo vertex_shader_stage_info = {};
	vertex_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertex_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertex_shader_stage_info.module = vertex_shader_module;
	vertex_shader_stage_info.pName = "main";

	VkPipelineShaderStageCreateInfo fragment_shader_stage_info = {};
	fragment_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragment_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragment_shader_stage_info.module = fragment_shader_module;
	fragment_shader_stage_info.pName = "main";

	// Allows for easy access when creating the pipeline
	VkPipelineShaderStageCreateInfo pipeline_shader_stages[] =
	{
		vertex_shader_stage_info,
		fragment_shader_stage_info
	};

	// Describe the vertex data format
	const auto binding_description = Vertex::GetBindingDescription();
	const auto attribute_descriptions = Vertex::GetAttributeDescriptions();
	
	VkPipelineVertexInputStateCreateInfo vertex_input_state = {};
	vertex_input_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertex_input_state.vertexBindingDescriptionCount = 1;
	vertex_input_state.pVertexBindingDescriptions = &binding_description;
	vertex_input_state.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribute_descriptions.size());
	vertex_input_state.pVertexAttributeDescriptions = attribute_descriptions.data();

	// Describe geometry and if primitive restart should be enabled
	VkPipelineInputAssemblyStateCreateInfo input_assembly_state = {};
	input_assembly_state.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly_state.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	input_assembly_state.primitiveRestartEnable = VK_FALSE;

	// Configure the viewport
	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(m_swapchain_extent.width);
	viewport.height = static_cast<float>(m_swapchain_extent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	// Configure the scissor rectangle
	VkRect2D scissor_rect = {};
	scissor_rect.offset = { 0, 0 };
	scissor_rect.extent = m_swapchain_extent;

	// Combine the viewport and scissor rectangle settings into a viewport structure
	VkPipelineViewportStateCreateInfo viewport_state = {};
	viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport_state.viewportCount = 1;
	viewport_state.pViewports = &viewport;
	viewport_state.scissorCount = 1;
	viewport_state.pScissors = &scissor_rect;

	// Configure the rasterizer
	VkPipelineRasterizationStateCreateInfo rasterization_state = {};
	rasterization_state.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterization_state.depthClampEnable = VK_FALSE;
	rasterization_state.rasterizerDiscardEnable = VK_FALSE;
	rasterization_state.polygonMode = VK_POLYGON_MODE_FILL;
	rasterization_state.lineWidth = 1.0f;
	rasterization_state.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterization_state.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterization_state.depthBiasEnable = VK_FALSE;

	// Configure multi sampling
	VkPipelineMultisampleStateCreateInfo multisample_state = {};
	multisample_state.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisample_state.sampleShadingEnable = VK_FALSE;
	multisample_state.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	// Color blending
	VkPipelineColorBlendAttachmentState color_blend_attachment = {};
	color_blend_attachment.colorWriteMask =
		VK_COLOR_COMPONENT_R_BIT |
		VK_COLOR_COMPONENT_G_BIT |
		VK_COLOR_COMPONENT_B_BIT |
		VK_COLOR_COMPONENT_A_BIT;
	color_blend_attachment.blendEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo color_blend_state = {};
	color_blend_state.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	color_blend_state.logicOpEnable = VK_FALSE;
	color_blend_state.attachmentCount = 1;
	color_blend_state.pAttachments = &color_blend_attachment;

	// Pipeline layout
	VkPipelineLayoutCreateInfo pipeline_layout_info = {};
	pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_info.setLayoutCount = 1;
	pipeline_layout_info.pSetLayouts = &m_camera_data_descriptor_set_layout;

	if (vkCreatePipelineLayout(m_device, &pipeline_layout_info, nullptr, &m_pipeline_layout) != VK_SUCCESS)
		spdlog::error("Could not create a pipeline layout.");

	spdlog::info("Successfully created a pipeline layout.");

	// Create the graphics pipeline
	VkGraphicsPipelineCreateInfo pipeline_info = {};
	pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipeline_info.stageCount = 2;
	pipeline_info.pStages = pipeline_shader_stages;
	pipeline_info.pVertexInputState = &vertex_input_state;
	pipeline_info.pInputAssemblyState = &input_assembly_state;
	pipeline_info.pViewportState = &viewport_state;
	pipeline_info.pRasterizationState = &rasterization_state;
	pipeline_info.pMultisampleState = &multisample_state;
	pipeline_info.pColorBlendState = &color_blend_state;
	pipeline_info.layout = m_pipeline_layout;
	pipeline_info.renderPass = m_render_pass;
	pipeline_info.subpass = 0;

	if (vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &m_graphics_pipeline) != VK_SUCCESS)
		spdlog::error("Could not create a graphics pipeline.");
	else
		spdlog::info("Successfully created a graphics pipeline.");

	// Get rid of the shader modules, as they are no longer needed after the
	// pipeline has been created
	vkDestroyShaderModule(m_device, vertex_shader_module, nullptr);
	vkDestroyShaderModule(m_device, fragment_shader_module, nullptr);
}

VkShaderModule Renderer::CreateShaderModule(const std::vector<char>& spirv)
{
	VkShaderModule shader_module;
	VkShaderModuleCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	create_info.codeSize = spirv.size();
	create_info.pCode = reinterpret_cast<const uint32_t*>(spirv.data());

	if (vkCreateShaderModule(m_device, &create_info, nullptr, &shader_module) != VK_SUCCESS)
	{
		spdlog::error("Could not create a shader module.");
		return VK_NULL_HANDLE;
	}

	spdlog::info("Shader module created successfully.");

	return shader_module;
}

void Renderer::CreateRenderPass()
{
	VkAttachmentDescription color_attachment = {};
	color_attachment.format = m_swapchain_format;
	color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference color_attachment_ref = {};
	color_attachment_ref.attachment = 0;
	color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &color_attachment_ref;

	VkSubpassDependency subpass_dependency = {};
	subpass_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	subpass_dependency.dstSubpass = 0;
	subpass_dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpass_dependency.srcAccessMask = 0;
	subpass_dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpass_dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo render_pass_info = {};
	render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	render_pass_info.attachmentCount = 1;
	render_pass_info.pAttachments = &color_attachment;
	render_pass_info.subpassCount = 1;
	render_pass_info.pSubpasses = &subpass;
	render_pass_info.dependencyCount = 1;
	render_pass_info.pDependencies = &subpass_dependency;

	if (vkCreateRenderPass(m_device, &render_pass_info, nullptr, &m_render_pass) != VK_SUCCESS)
	{
		spdlog::error("Could not create a render pass.");
		return;
	}

	spdlog::info("Successfully created a render pass.");
}

void Renderer::CreateFramebuffers()
{
	// Allocate enough memory to hold all framebuffers for the swapchain
	m_swapchain_framebuffers.resize(m_swapchain_image_views.size());

	// Index for the for-loop
	uint32_t index = 0;

	// Create a new framebuffer for each image view
	for (const auto& image_view : m_swapchain_image_views)
	{
		VkFramebufferCreateInfo framebuffer_info = {};
		framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebuffer_info.renderPass = m_render_pass;
		framebuffer_info.attachmentCount = 1;
		framebuffer_info.pAttachments = &image_view;
		framebuffer_info.width = m_swapchain_extent.width;
		framebuffer_info.height = m_swapchain_extent.height;
		framebuffer_info.layers = 1;

		if (vkCreateFramebuffer(m_device, &framebuffer_info, nullptr, &m_swapchain_framebuffers[index]) != VK_SUCCESS)
			spdlog::error("Could not create a framebuffer for the swapchain image view.");

		++index;
	}

	spdlog::info("Successfully created a framebuffer for each swapchain image view.");
}

void Renderer::CreateCommandPools()
{
	VkCommandPoolCreateInfo graphics_pool_info = {};
	graphics_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	graphics_pool_info.queueFamilyIndex = m_queue_family_indices.graphics_family_index->first;

	VkCommandPoolCreateInfo transfer_pool_info = {};
	transfer_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;

	// Choose the correct queue family index if there is a dedicated transfer queue on this system
	if (m_queue_family_indices.transfer_family_index.has_value())
		transfer_pool_info.queueFamilyIndex = m_queue_family_indices.transfer_family_index->first;
	else
		transfer_pool_info.queueFamilyIndex = m_queue_family_indices.graphics_family_index->first;

	if (vkCreateCommandPool(m_device, &graphics_pool_info, nullptr, &m_graphics_command_pool) != VK_SUCCESS)
	{
		spdlog::error("Could not create a graphics command pool.");
		return;
	}
	else
	{
		spdlog::info("Successfully created a graphics command pool.");
	}

	if (vkCreateCommandPool(m_device, &transfer_pool_info, nullptr, &m_transfer_command_pool) != VK_SUCCESS)
	{
		spdlog::error("Could not create a transfer command pool.");
	}
	else
	{
		spdlog::info("Successfully created a transfer command pool.");
	}
}

void Renderer::CreateCommandBuffers()
{
	// We need one command buffer per swapchain framebuffer
	m_command_buffers.resize(m_swapchain_framebuffers.size());
	
	VkCommandBufferAllocateInfo graphics_command_buffer_allocate_info = {};
	graphics_command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	graphics_command_buffer_allocate_info.commandPool = m_graphics_command_pool;
	graphics_command_buffer_allocate_info.commandBufferCount = static_cast<uint32_t>(m_command_buffers.size());
	graphics_command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

	// Allocate the command buffers
	if (vkAllocateCommandBuffers(m_device, &graphics_command_buffer_allocate_info, m_command_buffers.data()) != VK_SUCCESS)
	{
		spdlog::error("Could not allocate command buffers.");
		return;
	}

	spdlog::info("Successfully allocated command buffers.");

	// Index tracking for the range-based for-loop
	uint32_t index = 0;

	// Record commands into command buffers
	for (const auto& command_buffer : m_command_buffers)
	{
		VkCommandBufferBeginInfo begin_info = {};
		begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
		
		// Begin recording
		if (vkBeginCommandBuffer(command_buffer, &begin_info) != VK_SUCCESS)
		{
			spdlog::error("Could not begin recording to command buffer #{}.", index);
			return;
		}

		// Black clear color
		VkClearValue clear_color = { 0.0f, 0.0f, 0.0f, 1.0f };

		// Prepare the render pass
		VkRenderPassBeginInfo render_pass_begin_info = {};
		render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		render_pass_begin_info.renderPass = m_render_pass;
		render_pass_begin_info.framebuffer = m_swapchain_framebuffers[index];
		render_pass_begin_info.renderArea.offset = { 0, 0 };
		render_pass_begin_info.renderArea.extent = m_swapchain_extent;
		render_pass_begin_info.clearValueCount = 1;
		render_pass_begin_info.pClearValues = &clear_color;

		// Start the render pass
		vkCmdBeginRenderPass(m_command_buffers[index], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

		// Bind the graphics pipeline
		vkCmdBindPipeline(m_command_buffers[index], VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphics_pipeline);

		// Bind the triangle vertex buffer
		VkBuffer vertex_buffers[] = { m_vertex_buffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(m_command_buffers[index], 0, 1, vertex_buffers, offsets);

		// Draw the triangle using hard-coded shader vertices
		vkCmdDraw(m_command_buffers[index], static_cast<uint32_t>(vertices.size()), 1, 0, 0);

		// End the render pass
		vkCmdEndRenderPass(m_command_buffers[index]);

		// Finish recording
		if (vkEndCommandBuffer(m_command_buffers[index]) != VK_SUCCESS)
		{
			spdlog::error("Could not record to command buffer #{}.", index);
			return;
		}

		++index;
	}
}

void Renderer::CreateSynchronizationObjects()
{
	m_in_flight_frame_image_available_semaphores.resize(global_settings::maximum_in_flight_frame_count);
	m_in_flight_render_finished_semaphores.resize(global_settings::maximum_in_flight_frame_count);
	m_in_flight_fences.resize(global_settings::maximum_in_flight_frame_count);

	VkSemaphoreCreateInfo semaphore_create_info = {};
	semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fence_create_info = {};
	fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;	// Fence is set to signaled on creation

	// Create all required semaphores
	for (auto index = 0; index < global_settings::maximum_in_flight_frame_count; ++index)
	{
		if (vkCreateSemaphore(m_device, &semaphore_create_info, nullptr, &m_in_flight_frame_image_available_semaphores[index]) != VK_SUCCESS ||
			vkCreateSemaphore(m_device, &semaphore_create_info, nullptr, &m_in_flight_render_finished_semaphores[index]) != VK_SUCCESS)
		{
			spdlog::error("Could not create one or both semaphores.");
			return;
		}
	}

	// Create all required fences
	for (auto index = 0; index < global_settings::maximum_in_flight_frame_count; ++index)
	{
		if (vkCreateFence(m_device, &fence_create_info, nullptr, &m_in_flight_fences[index]) != VK_SUCCESS)
		{
			spdlog::error("Could not create a fence.");
			return;
		}
	}
}

void Renderer::RecreateSwapchain()
{
	int width = 0;
	int height = 0;

	while (width == 0 || height == 0)
	{
		glfwGetFramebufferSize(m_window, &width, &height);
		glfwWaitEvents();
	}

	// Wait until the GPU finishes and clean-up the, now outdated, swapchain
	vkDeviceWaitIdle(m_device);
	CleanUpSwapchain();

	// Create a new swapchain
	QuerySwapchainSupport();
	CreateSwapchain();
	CreateSwapchainImageViews();
	CreateRenderPass();
	CreateGraphicsPipeline();
	CreateFramebuffers();
	CreateUniformBuffers();
	CreateDescriptorPool();
	CreateDescriptorSets();
	CreateCommandBuffers();

	spdlog::info("Recreated the swapchain successfully.");
}

void Renderer::CleanUpSwapchain()
{
	for (auto index = 0; index < m_swapchain_images.size(); ++index)
	{
		vkDestroyFramebuffer(m_device, m_swapchain_framebuffers[index], nullptr);
		vkDestroyBuffer(m_device, m_camera_ubos[index], nullptr);
		vkFreeMemory(m_device, m_camera_ubos_memory[index], nullptr);
	}

	vkDestroyDescriptorPool(m_device, m_descriptor_pool, nullptr);

	// No need to recreate the pool, freeing the command buffers is enough
	vkFreeCommandBuffers(m_device, m_graphics_command_pool, static_cast<uint32_t>(m_command_buffers.size()), m_command_buffers.data());

	vkDestroyPipeline(m_device, m_graphics_pipeline, nullptr);
	vkDestroyPipelineLayout(m_device, m_pipeline_layout, nullptr);
	vkDestroyRenderPass(m_device, m_render_pass, nullptr);

	for (const auto& image_view : m_swapchain_image_views)
	{
		vkDestroyImageView(m_device, image_view, nullptr);
	}

	vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
}

void Renderer::CreateTransferCommandBuffer()
{
	VkCommandBufferAllocateInfo transfer_command_buffer_allocate_info = {};
	transfer_command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	transfer_command_buffer_allocate_info.commandPool = m_transfer_command_pool;
	transfer_command_buffer_allocate_info.commandBufferCount = 1;
	transfer_command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

	if (vkAllocateCommandBuffers(m_device, &transfer_command_buffer_allocate_info, &m_transfer_command_buffer) != VK_SUCCESS)
		spdlog::error("Could not allocate a transfer command buffer.");
	else
		spdlog::info("Successfully allocated a transfer command buffer.");
}

void Renderer::CreateVertexBuffer()
{
	VkDeviceSize buffer_size = sizeof(Vertex) * vertices.size();

	VkBuffer staging_buffer;
	VkDeviceMemory staging_buffer_memory;

	// Create a staging buffer
	CreateBuffer(
		buffer_size,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		staging_buffer,
		staging_buffer_memory,
		m_queue_family_indices,
		m_device,
		m_physical_device);

	void* data = nullptr;
	
	// Copy the data to a staging buffer
	vkMapMemory(m_device, staging_buffer_memory, 0, buffer_size, 0, &data);
	memcpy(data, vertices.data(), static_cast<size_t>(buffer_size));
	vkUnmapMemory(m_device, staging_buffer_memory);

	// Create a device local buffer to hold the vertex data in GPU memory
	CreateBuffer(
		buffer_size,
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		m_vertex_buffer,
		m_vertex_buffer_memory,
		m_queue_family_indices,
		m_device,
		m_physical_device);

	// Copy the staging buffer to the GPU visible buffer
	CopyStagingBufferToDeviceLocalBuffer(
		staging_buffer,
		m_vertex_buffer,
		buffer_size,
		m_transfer_command_buffer,
		m_transfer_queue);

	// Clean up the staging buffer since it is no longer needed
	vkDestroyBuffer(m_device, staging_buffer, nullptr);
	vkFreeMemory(m_device, staging_buffer_memory, nullptr);
}

void Renderer::CreateUniformBuffers()
{
	VkDeviceSize ubo_size = sizeof(CameraData);

	m_camera_ubos.resize(m_swapchain_images.size());
	m_camera_ubos_memory.resize(m_swapchain_images.size());

	// Create a camera data UBO for each image in the swapchain
	for (auto index = 0; index < m_swapchain_images.size(); ++index)
	{
		CreateBuffer(
			ubo_size,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			m_camera_ubos[index],
			m_camera_ubos_memory[index],
			m_queue_family_indices,
			m_device,
			m_physical_device);
	}
}

void Renderer::CreateDescriptorPool()
{
	VkDescriptorPoolSize descriptor_pool_size = {};
	descriptor_pool_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptor_pool_size.descriptorCount = static_cast<uint32_t>(m_swapchain_images.size());

	VkDescriptorPoolCreateInfo pool_create_info = {};
	pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_create_info.poolSizeCount = 1;
	pool_create_info.pPoolSizes = &descriptor_pool_size;
	pool_create_info.maxSets = static_cast<uint32_t>(m_swapchain_images.size());

	if (vkCreateDescriptorPool(m_device, &pool_create_info, nullptr, &m_descriptor_pool) != VK_SUCCESS)
	{
		spdlog::error("Could not create a descriptor pool.");
		return;
	}

	spdlog::info("Successfully created a descriptor pool.");
}

void Renderer::CreateDescriptorSetLayout()
{
	VkDescriptorSetLayoutBinding camera_data_layout_binding = {};
	camera_data_layout_binding.binding = 0;
	camera_data_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	camera_data_layout_binding.descriptorCount = 1;
	camera_data_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkDescriptorSetLayoutCreateInfo layout_create_info = {};
	layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layout_create_info.bindingCount = 1;
	layout_create_info.pBindings = &camera_data_layout_binding;

	if (vkCreateDescriptorSetLayout(m_device, &layout_create_info, nullptr, &m_camera_data_descriptor_set_layout) != VK_SUCCESS)
		spdlog::error("Could not create a descriptor set layout for the camera data.");
}

void Renderer::CreateDescriptorSets()
{
	LEFT OFF HERE: https://vulkan-tutorial.com/Uniform_buffers/Descriptor_pool_and_sets
}

uint32_t Renderer::FindMemoryType(
	uint32_t type_filter,
	VkMemoryPropertyFlags properties,
	const VkPhysicalDevice& physical_device)
{
	VkPhysicalDeviceMemoryProperties memory_properties;
	vkGetPhysicalDeviceMemoryProperties(physical_device, &memory_properties);

	for (uint32_t index = 0; index < memory_properties.memoryTypeCount; ++index)
	{
		// Iterate through the types and check when the "type_filter" bit field is set to 1
		// Also check whether all required properties are supported
		if ((type_filter & (1 << index)) && (memory_properties.memoryTypes[index].propertyFlags & properties) == properties)
			return index;
	}

	spdlog::error("Could not find a suitable memory type");
	return 0;
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

std::vector<char> Renderer::ReadSPRIVFromFile(const char* file)
{
	std::ifstream source_code(file, std::ios::ate | std::ios::binary);

	if (!source_code.is_open())
	{
		spdlog::error("Could not open \"{}\".", file);
		return std::vector<char>();
	}

	// Get the size of the entire shader source file (the file is opened with the position set to end, so this will
	// return the size of the entire file
	auto shader_code_size = source_code.tellg();

	std::vector<char> spirv(shader_code_size);

	// Move to the beginning of the file
	source_code.seekg(0);
	source_code.read(spirv.data(), shader_code_size);

	spdlog::info("Successfully read the shader \"{}\".", file);

	return spirv;
}

void Renderer::CreateBuffer(
	VkDeviceSize size,
	VkBufferUsageFlags usage,
	VkMemoryPropertyFlags properties,
	VkBuffer& buffer,
	VkDeviceMemory& buffer_memory,
	const QueueFamilyIndices& queue_family_indices,
	const VkDevice& device,
	const VkPhysicalDevice physical_device)
{
	VkBufferCreateInfo buffer_info = {};
	buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_info.size = size;
	buffer_info.usage = usage;

	if (queue_family_indices.transfer_family_index.has_value())
	{
		// Use a dedicated transfer queue
		uint32_t queue_indices[] =
		{
			queue_family_indices.graphics_family_index->first,
			queue_family_indices.transfer_family_index->first
		};

		buffer_info.sharingMode = VK_SHARING_MODE_CONCURRENT;
		buffer_info.queueFamilyIndexCount = sizeof(queue_indices) / sizeof(queue_indices[0]);
		buffer_info.pQueueFamilyIndices = queue_indices;
	}
	else
	{
		// Use the graphics queue as a transfer queue as well
		buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		buffer_info.queueFamilyIndexCount = 1;
		buffer_info.pQueueFamilyIndices = &queue_family_indices.graphics_family_index->first;
	}

	if (vkCreateBuffer(device, &buffer_info, nullptr, &buffer) != VK_SUCCESS)
	{
		spdlog::error("Could not create a vertex buffer.");
		return;
	}

	VkMemoryRequirements memory_requirements;
	vkGetBufferMemoryRequirements(device, buffer, &memory_requirements);

	VkMemoryAllocateInfo allocation_info = {};
	allocation_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocation_info.allocationSize = memory_requirements.size;
	allocation_info.memoryTypeIndex = FindMemoryType(
		memory_requirements.memoryTypeBits,
		properties,
		physical_device);

	if (vkAllocateMemory(device, &allocation_info, nullptr, &buffer_memory) != VK_SUCCESS)
	{
		spdlog::error("Could not allocate memory for the vertex buffer.");
		return;
	}

	// Associate the memory with the buffer
	vkBindBufferMemory(device, buffer, buffer_memory, 0);
}

void Renderer::CopyStagingBufferToDeviceLocalBuffer(
	const VkBuffer& source,
	const VkBuffer& destination,
	VkDeviceSize size,
	const VkCommandBuffer& transfer_command_buffer,
	const VkQueue& transfer_queue)
{
	VkCommandBufferBeginInfo begin_info = {};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	// Begin recording to the command buffer
	vkBeginCommandBuffer(transfer_command_buffer, &begin_info);

	VkBufferCopy copy_region = {};
	copy_region.size = size;

	// Issue a command that copies the staging buffer to the destination buffer
	vkCmdCopyBuffer(transfer_command_buffer, source, destination, 1, &copy_region);

	// End recording to the command buffer
	vkEndCommandBuffer(transfer_command_buffer);

	// Upload the staging buffer to the GPU by executing the transfer command buffer
	VkSubmitInfo submit_info = {};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &transfer_command_buffer;

	// Execute the commands
	vkQueueSubmit(transfer_queue, 1, &submit_info, VK_NULL_HANDLE);
	vkQueueWaitIdle(transfer_queue);
}
