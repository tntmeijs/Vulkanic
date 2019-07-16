// Application
#include "miscellaneous/exceptions.hpp"
#include "vulkan_device.hpp"
#include "vulkan_instance.hpp"
#include "vulkan_swapchain.hpp"
#include "vulkan_utility.hpp"

// Spdlog
#include <spdlog/spdlog.h>

// C++ standard
#include <algorithm>
#include <set>

using namespace vkc::vk_wrapper;

void VulkanDevice::Create(
	const VulkanInstance& instance,
	const VulkanSwapchain& swapchain,
	const std::vector<std::string>& extensions) noexcept(false)
{
	// Get the best physical device available on this machine
	SelectPhysicalDevice(instance, extensions);

	// Find queue all queue families
	FindQueueFamilyIndices(swapchain);

	// Check if all required queue family indices were found
	if (!m_queue_family_indices.IsComplete())
	{
		throw exception::CriticalVulkanError("Queue family indices incomplete.");
	}

	// Create the logical device
	CreateLogicalDevice(extensions);

	// Save handles to the queues
	vkGetDeviceQueue(
		m_logical_device,
		m_queue_family_indices.graphics_family_index->first,
		0,
		&m_graphics_queue);

	vkGetDeviceQueue(
		m_logical_device,
		m_queue_family_indices.present_family_index->first,
		0,
		&m_present_queue);

	vkGetDeviceQueue(
		m_logical_device,
		m_queue_family_indices.compute_family_index->first,
		0,
		&m_compute_queue);
}

void VulkanDevice::Destroy() const noexcept(true)
{
	vkDestroyDevice(m_logical_device, nullptr);
}

const VkPhysicalDevice& VulkanDevice::GetPhysicalDeviceNative() const noexcept(true)
{
	return m_physical_device;
}

const VkDevice& VulkanDevice::GetLogicalDeviceNative() const noexcept(true)
{
	return m_logical_device;
}

const QueueFamilyIndices& VulkanDevice::GetQueueFamilyIndices() const noexcept(true)
{
	return m_queue_family_indices;
}

const VkQueue& VulkanDevice::GetQueueNativeOfType(VulkanQueueType queue_type) const noexcept(false)
{
	switch (queue_type)
	{
		case VulkanQueueType::Graphics:
			return m_graphics_queue;
			break;
		
		case VulkanQueueType::Present:
			return m_present_queue;
			break;
		
		case VulkanQueueType::Compute:
			return m_compute_queue;
			break;

		default:
			throw exception::CriticalVulkanError("Invalid queue type requested.");
			break;
	}
}

void VulkanDevice::SelectPhysicalDevice(
	const VulkanInstance& instance,
	const std::vector<std::string> extensions) noexcept(false)
{
	std::uint32_t physical_device_count = 0;
	vkEnumeratePhysicalDevices(instance.GetNative(), &physical_device_count, nullptr);

	if (physical_device_count == 0)
	{
		throw exception::CriticalVulkanError("No physical devices found.");
	}

	std::vector<VkPhysicalDevice> available_devices(physical_device_count);
	vkEnumeratePhysicalDevices(
		instance.GetNative(),
		&physical_device_count,
		available_devices.data());

	// Choose the best physical device
	auto physical_device = FindBestPhysicalDevice(available_devices);

	// Check if all extensions are supported
	std::uint32_t extension_count = 0;
	vkEnumerateDeviceExtensionProperties(
		physical_device,
		nullptr,
		&extension_count,
		nullptr);

	if (extensions.empty())
	{
		// Most Vulkan applications use at least one extension
		spdlog::warn("No device extensions specified, are you 100% sure this is intended?");
	}

	// No extensions available on this device
	if (extension_count == 0 && !extensions.empty())
	{
		throw exception::CriticalVulkanError("No device extensions available.");
	}

	std::vector<VkExtensionProperties> available_extensions(extension_count);
	vkEnumerateDeviceExtensionProperties(
		physical_device,
		nullptr,
		&extension_count,
		available_extensions.data());

	// Save the extension names
	std::vector<std::string> available_extension_names;
	for (const auto& extension : available_extensions)
	{
		available_extension_names.push_back(extension.extensionName);
	}

	// Check whether all required extensions are supported
	if (!utility::AllRequiredItemsExistInVector(extensions, available_extension_names))
	{
		throw exception::CriticalVulkanError("Not every device extension is supported.");
	}

	m_physical_device = physical_device;
}

VkPhysicalDevice VulkanDevice::FindBestPhysicalDevice(
	const std::vector<VkPhysicalDevice>& devices) const noexcept(false)
{
	std::vector<std::pair<std::uint32_t, VkPhysicalDevice>> scores;

	for (const auto& device : devices)
	{
		std::uint32_t score = 0;

		VkPhysicalDeviceMemoryProperties memory_properties = {};
		VkPhysicalDeviceProperties properties = {};

		vkGetPhysicalDeviceMemoryProperties(device, &memory_properties);
		vkGetPhysicalDeviceProperties(device, &properties);

		// Always prefer a discrete GPU
		if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		{
			score += 1000;
		}

		// More VRAM is better
		for (const auto& heap : memory_properties.memoryHeaps)
		{
			// Find the heap that represents the VRAM
			if (heap.flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
			{
				// Add the size in megabytes to the score to avoid a huge number
				score += static_cast<std::uint32_t>(heap.size / 1024 / 1024);
			}
		}

		//#TODO: Add more checks to better find the best suited device

		scores.push_back({ score, device });
	}

	// Sort the devices in descending order
	std::sort(scores.begin(), scores.end(), [](
		const std::pair<std::uint32_t, VkPhysicalDevice> & a,
		const std::pair<std::uint32_t, VkPhysicalDevice> & b)
	{
		return (a.first > b.first);
	});

	// If the score is 0, the device is unusable
	if (scores[0].first == 0)
	{
		throw exception::CriticalVulkanError("Invalid physical device score.");
	}

	// The first element is the best device
	return scores[0].second;
}

void VulkanDevice::FindQueueFamilyIndices(
	const VulkanSwapchain& swapchain) noexcept(false)
{
	std::uint32_t queue_family_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(
		m_physical_device,
		&queue_family_count,
		nullptr);

	if (queue_family_count == 0)
	{
		throw exception::CriticalVulkanError("No queue families available.");
	}

	std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
	vkGetPhysicalDeviceQueueFamilyProperties(
		m_physical_device,
		&queue_family_count,
		queue_families.data());

	std::uint32_t index = 0;
	for (const auto& queue_family : queue_families)
	{
		// Does this queue family support presenting?
		VkBool32 present_supported = true;
		vkGetPhysicalDeviceSurfaceSupportKHR(
			m_physical_device,
			index,
			swapchain.GetSurfaceNative(),
			&present_supported);

		// Look for a queue family that supports present operations
		if (present_supported)
		{
			m_queue_family_indices.present_family_index = { 0, 0 };
			m_queue_family_indices.present_family_index->first = index;
			m_queue_family_indices.present_family_index->second = queue_family.queueCount;
		}

		// Look for a queue family that supports graphics operations
		if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			m_queue_family_indices.graphics_family_index = { 0, 0 };
			m_queue_family_indices.graphics_family_index->first = index;
			m_queue_family_indices.graphics_family_index->second = queue_family.queueCount;
		}

		// Look for a queue family that supports compute operations
		if (queue_family.queueFlags & VK_QUEUE_COMPUTE_BIT)
		{
			m_queue_family_indices.compute_family_index = { 0, 0 };
			m_queue_family_indices.compute_family_index->first = index;
			m_queue_family_indices.compute_family_index->second = queue_family.queueCount;
		}

		// Stop searching once all queue family indices have been found
		if (m_queue_family_indices.IsComplete())
		{
			break;
		}

		++index;
	}
}

void VulkanDevice::CreateLogicalDevice(
	const std::vector<std::string>& extensions) noexcept(false)
{
	// Eliminate duplicate queue family indices
	std::set<std::uint32_t> unique_family_indices
	{
		m_queue_family_indices.graphics_family_index->first,
		m_queue_family_indices.present_family_index->first,
		m_queue_family_indices.compute_family_index->first
	};

	// Hold a create info structure per queue family index
	std::vector<VkDeviceQueueCreateInfo> queue_infos;
	queue_infos.reserve(unique_family_indices.size());

	// One create info per unique queue
	for (const auto unique_index : unique_family_indices)
	{
		float queue_priority = 1.0f;

		VkDeviceQueueCreateInfo queue_create_info = {};
		queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queue_create_info.queueFamilyIndex = unique_index;
		queue_create_info.queueCount = 1;
		queue_create_info.pQueuePriorities = &queue_priority;

		queue_infos.push_back(queue_create_info);
	}

	// Get all physical device features
	VkPhysicalDeviceFeatures device_features;
	vkGetPhysicalDeviceFeatures(m_physical_device, &device_features);

	// The create info below needs a c-string instead of std::string
	auto extension_names_cstring = utility::ConvertVectorOfStringsToCString(extensions);

	// Create the logical device
	VkDeviceCreateInfo device_info = {};
	device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	device_info.pQueueCreateInfos = queue_infos.data();
	device_info.queueCreateInfoCount = static_cast<std::uint32_t>(queue_infos.size());
	device_info.pEnabledFeatures = &device_features;
	device_info.enabledExtensionCount = static_cast<std::uint32_t>(extensions.size());
	device_info.ppEnabledExtensionNames = extension_names_cstring.data();

	auto result = vkCreateDevice(m_physical_device, &device_info, nullptr, &m_logical_device);
	
	if (result != VK_SUCCESS)
	{
		throw exception::CriticalVulkanError("Could not create a logical device.");
	}
}
