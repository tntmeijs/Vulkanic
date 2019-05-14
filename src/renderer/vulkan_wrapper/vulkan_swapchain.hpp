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
			void Create(
				const VulkanDevice& device,
				const Window& window) noexcept(false);

			/** Destroy the swapchain surface */
			void DestroySurface(const VulkanInstance& instance) const noexcept(true);

			/** Get a reference to the surface object */
			const VkSurfaceKHR& GetSurfaceNative() const noexcept(true);

			/** Destroy the swapchain */
			void Destroy(const VulkanDevice& device) const noexcept(true);

			/** Get a reference to the Vulkan swapchain object */
			const VkSwapchainKHR& GetNative() const noexcept(true);

			/** Get a reference to the swapchain format */
			const VkFormat& GetFormat() const noexcept(true);

			/** Get a reference to the swapchain extent */
			const VkExtent2D& GetExtent() const noexcept(true);

			/** Get a reference to the swapchain images */
			const std::vector<VkImage>& GetImages() const noexcept(true);

			/** Get a reference to the swapchain image views */
			const std::vector<VkImageView>& GetImageViews() const noexcept(true);

		private:
			/** Check whether a valid swapchain can be created */
			SwapchainSupportDetails QuerySwapchainSupport(
				const VulkanDevice& device) const noexcept(false);

			/** Create the Vulkan swapchain object */
			/**
			 * Creates a swapchain. This function essentially groups
			 * all swapchain creation steps into one big function.
			 *
			 * The "out_format" and "out_extent" parameters output
			 * the swapchain surface format and extent. These values
			 * should be stored for later use...
			*/
			void CreateSwapchain(
				const VulkanDevice& device,
				const Window& window) noexcept(false);

			/** Find the best suited swapchain surface format */
			VkSurfaceFormatKHR FindBestSurfaceFormat() const noexcept(true);

			/** Find a proper surface extent */
			VkExtent2D FindSurfaceExtent(
				const Window& window) const noexcept(true);

			/** Find the best suited swapchains urface present mode */
			VkPresentModeKHR FindBestSurfacePresentMode() const noexcept(true);

			/** Get a vector of handles to the swapchain images */
			void GetSwapchainImages(
				const VulkanDevice& device) noexcept(true);

			/** Create an image view for each swapchain image */
			void CreateSwapchainImagesImageViews(
				const VulkanDevice& device) noexcept(false);

			/** Clean-up swapchain resources */
			void DestroySwapchainResources(
				const VulkanDevice& device) const noexcept(true);

		private:
			VkSurfaceKHR m_surface;
			VkSwapchainKHR m_swapchain;
			VkFormat m_swapchain_format;
			VkExtent2D m_swapchain_extent;

			std::vector<VkImage> m_swapchain_images;
			std::vector<VkImageView> m_swapchain_image_views;

			SwapchainSupportDetails m_support_details;
		};
	}
}

#endif
