#ifndef VULKAN_DEVICE_HPP
#define VULKAN_DEVICE_HPP

// Vulkan
#include <vulkan/vulkan.h>

namespace vkc::vk_wrapper
{
	class VulkanDevice
	{
	public:
		VulkanDevice() noexcept(true) : m_logical_device(VK_NULL_HANDLE), m_physical_device(VK_NULL_HANDLE) {}
		~VulkanDevice() noexcept(true) {}

	private:
		VkDevice m_logical_device;
		VkPhysicalDevice m_physical_device;
	};
}

#endif
