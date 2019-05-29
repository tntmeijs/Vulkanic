#ifndef VULKAN_COMMAND_POOL_HPP
#define VULKAN_COMMAND_POOL_HPP

// Vulkan
#include <vulkan/vulkan.h>

namespace vkc::vk_wrapper
{
	class VulkanDevice;

	enum class CommandPoolType
	{
		Graphics,
		Compute
	};

    class VulkanCommandPool
    {
	public:
		VulkanCommandPool() noexcept(true);
		~VulkanCommandPool() noexcept(true);

		/** Create a command pool */
		void Create(const VulkanDevice& device, CommandPoolType type) noexcept(false);

		/** Deallocate used resources */
		void Destroy(const VulkanDevice& device) const noexcept(true);

		/** Get a reference to the Vulkan command pool object */
		const VkCommandPool& GetNative() const noexcept(true);

	private:
		VkCommandPool m_command_pool;
    };
}

#endif
