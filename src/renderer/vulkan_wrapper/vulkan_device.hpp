#ifndef VULKAN_DEVICE_HPP
#define VULKAN_DEVICE_HPP

// Application
#include "vulkan_enums.hpp"
#include "vulkan_structures.hpp"

// Vulkan
#include <vulkan/vulkan.h>

// C++ standard
#include <optional>
#include <string>
#include <vector>

namespace vkc::vk_wrapper
{
	class VulkanInstance;
	class VulkanSwapchain;
	
	/** Queue family indices information */
	struct QueueFamilyIndices
	{
		// pair::first = index, pair::second = queue count
		std::optional<std::pair<uint32_t, uint32_t>> graphics_family_index;

		// pair::first = index, pair::second = queue count
		std::optional<std::pair<uint32_t, uint32_t>> present_family_index;

		// pair::first = index, pair::second = queue count
		std::optional<std::pair<uint32_t, uint32_t>> compute_family_index;

		bool IsComplete()
		{
			return (graphics_family_index.has_value() &&
				present_family_index.has_value() &&
				compute_family_index.has_value());
		}
	};

	class VulkanDevice
	{
	public:
		VulkanDevice() noexcept(true) : m_logical_device(VK_NULL_HANDLE), m_physical_device(VK_NULL_HANDLE) {}
		~VulkanDevice() noexcept(true) {}

		/** Create a physical device and a logical device */
		void Create(
			const VulkanInstance& instance,
			const VulkanSwapchain& swapchain,
			const std::vector<std::string>& extensions) noexcept(false);

		/** Destroy the logical device */
		/**
		 * Physical devices are not allocated by the application explicitly,
		 * which means that only the logical device needs to be destroyed.
		 */
		void Destroy() const noexcept(true);

		/** Get a reference to the physical device object */
		const VkPhysicalDevice& GetPhysicalDeviceNative() const noexcept(true);

		/** Get a reference to the logical device object */
		const VkDevice& GetLogicalDeviceNative() const noexcept(true);

		/** Get a reference to the queue family indices */
		const QueueFamilyIndices& GetQueueFamilyIndices() const noexcept(true);

		/** Get a reference to the requested queue */
		const VkQueue& GetQueueNativeOfType(enums::VulkanQueueType queue_type) const noexcept(false);

	private:
		/** Select and create a physical device */
		void SelectPhysicalDevice(
			const VulkanInstance& instance,
			const std::vector<std::string> extensions) noexcept(false);

		/** Get the best physical device available */
		VkPhysicalDevice FindBestPhysicalDevice(
			const std::vector<VkPhysicalDevice>& devices) const noexcept(false);

		/** Fills out the "QueueFamilyIndices" structure */
		void FindQueueFamilyIndices(
			const VulkanSwapchain& swapchain) noexcept(false);

		/** Create a logical device */
		void CreateLogicalDevice(
			const std::vector<std::string>& extensions) noexcept(false);

	private:
		VkDevice m_logical_device;
		VkPhysicalDevice m_physical_device;
		VkQueue m_compute_queue;
		VkQueue m_graphics_queue;
		VkQueue m_present_queue;

		QueueFamilyIndices m_queue_family_indices;
	};
}

#endif
