#ifndef VULKAN_UNIFORM_BUFFER_HPP
#define VULKAN_UNIFORM_BUFFER_HPP

// Vulkan
#include <vulkan/vulkan.h>

namespace vkc::vk_wrapper
{
	class VulkanUniformBuffer
	{
	public:
		VulkanUniformBuffer() noexcept(true);
		~VulkanUniformBuffer() noexcept(true);
	};
}

#endif // VULKAN_UNIFORM_BUFFER_HPP
