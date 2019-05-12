#ifndef VULKAN_SWAPCHAIN_HPP
#define VULKAN_SWAPCHAIN_HPP

// Vulkan
#include <vulkan/vulkan.h>

// C++ standard
#include <vector>

namespace vkc
{
	class Window;

	struct SwapchainSupportDetails
	{
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> present_modes;
	};

	namespace vk_wrapper
	{
		class VulkanDevice;
		class VulkanInstance;

		class VulkanSwapchain
		{
		public:
			VulkanSwapchain() noexcept(true) : m_surface(VK_NULL_HANDLE), m_swapchain(VK_NULL_HANDLE) {}
			~VulkanSwapchain() noexcept(true) {}

			/** Use GLFW to create a surface */
			void CreateSurface(
				const VulkanInstance& instance,
				const Window& window) noexcept(false);

			/** Create the swapchain */
			void Create(const VulkanDevice& device) noexcept(false);

			/** Destroy the swapchain surface */
			void DestroySurface(const VulkanInstance& instance) const noexcept(true);

			/** Get a reference to the surface object */
			const VkSurfaceKHR& GetSurfaceNative() const noexcept(true);

		private:
			/** Check whether a valid swapchain can be created */
			SwapchainSupportDetails QuerySwapchainSupport(
				const VulkanDevice& device) const noexcept(false);

			/** Create the Vulkan swapchain object */
			VkSwapchainKHR CreateSwapchain(
				const VulkanDevice& device,
				const SwapchainSupportDetails& support_details) const noexcept(false);

			/** Find the best suited swapchain surface format */
			VkSurfaceFormatKHR FindBestSurfaceFormat(
				const SwapchainSupportDetails& support_details) const noexcept(true);

		private:
			VkSurfaceKHR m_surface;
			VkSwapchainKHR m_swapchain;

			SwapchainSupportDetails m_support_details;
		};
	}
}

#endif
