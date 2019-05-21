#ifndef VULKAN_RENDER_PASS_HPP
#define VULKAN_RENDER_PASS_HPP

// Vulkan
#include <vulkan/vulkan.h>

namespace vkc::vk_wrapper
{
	class VulkanDevice;

	namespace structs
	{
		struct VulkanRenderPassInfo;
	}

	class VulkanRenderPass
	{
	public:
		VulkanRenderPass() noexcept(true) : m_render_pass(VK_NULL_HANDLE) {}
		~VulkanRenderPass() noexcept(true) {}

		/** Create a render pass using the information structure specified */
		void Create(
			const VulkanDevice& device,
			const structs::VulkanRenderPassInfo& info) noexcept(false);

		/** Destroy the Vulkan render pass object */
		void Destroy(const VulkanDevice& device) const noexcept(true);

		/** Get a reference to the Vulkan render pass object */
		const VkRenderPass& GetNative() const noexcept(true);

	private:
		VkRenderPass m_render_pass;
	};
}

#endif
