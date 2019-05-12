#ifndef VULKAN_SWAPCHAIN_HPP
#define VULKAN_SWAPCHAIN_HPP

// Vulkan
#include <vulkan/vulkan.h>

namespace vkc
{
	class Window;

	namespace vk_wrapper
	{
		class VulkanInstance;

		class VulkanSwapchain
		{
		public:
			VulkanSwapchain() noexcept(true) : m_surface(VK_NULL_HANDLE) {}
			~VulkanSwapchain() noexcept(true) {}

			/** Use GLFW to create a surface */
			void CreateSurface(
				const VulkanInstance& instance,
				const Window& window) noexcept(false);

			/** Destroy the swapchain surface */
			void DestroySurface(const VulkanInstance& instance) const noexcept(true);

			const VkSurfaceKHR& GetSurfaceNative() const noexcept(true);

		private:
			VkSurfaceKHR m_surface;
		};
	}
}

#endif
