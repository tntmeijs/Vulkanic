#ifndef VULKAN_FUNCTIONS_HPP
#define VULKAN_FUNCTIONS_HPP

// Application
#include "miscellaneous/exceptions.hpp"
#include "renderer/memory_manager/memory_manager.hpp"
#include "vulkan_command_buffer.hpp"
#include "vulkan_command_pool.hpp"
#include "vulkan_device.hpp"

// Vulkan
#include <vulkan/vulkan.h>

namespace vkc::vk_wrapper::func
{
	//////////////////////////////////////////////////////////////////////////

	/** Wraps Vulkan image creation */
	/**
	 * This function helps the user to create a Vulkan image. If image creation
	 * is successful, memory will be allocated as well.
	 */
	inline void CreateImage(
		const VkDevice& device,
		std::uint32_t width,
		std::uint32_t height,
		VkFormat format,
		VkImageTiling tiling,
		VkImageUsageFlags usage,
		VkImage& image) noexcept(false)
	{
		VkImageCreateInfo image_info = {};
		image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		image_info.imageType = VK_IMAGE_TYPE_2D;
		image_info.extent.width = width;
		image_info.extent.height = height;
		image_info.extent.depth = 1;
		image_info.mipLevels = 1;
		image_info.arrayLayers = 1;
		image_info.format = format;
		image_info.tiling = tiling;
		image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		image_info.usage = usage;
		image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		image_info.samples = VK_SAMPLE_COUNT_1_BIT;

		// Create the Vulkan image
		if (vkCreateImage(device, &image_info, nullptr, &image) != VK_SUCCESS)
		{
			throw exception::CriticalVulkanError("Could not create an image.");
		}
	}

	//////////////////////////////////////////////////////////////////////////

	/** Makes it easy to find the memory type index */
	inline std::uint32_t FindMemoryTypeIndex(
		std::uint32_t type_filter,
		const VkMemoryPropertyFlags& property_flags,
		const VkPhysicalDevice& physical_device) noexcept(false)
	{
		VkPhysicalDeviceMemoryProperties memory_properties;
		vkGetPhysicalDeviceMemoryProperties(physical_device, &memory_properties);

		for (std::uint32_t index = 0; index < memory_properties.memoryTypeCount; ++index)
		{
			// Iterate through the types and check when the "type_filter" bit field is set to 1
			// Also check whether all required properties are supported
			if ((type_filter & (1 << index)) &&
				(memory_properties.memoryTypes[index].propertyFlags & property_flags) == property_flags)
			{
				return index;
			}
		}

		// Failed to find s suitable memory type index
		throw exception::CriticalVulkanError("Failed to find a suitable memory type index.");
	}

	//////////////////////////////////////////////////////////////////////////

	/** Align a size to the specified alignment */
	inline std::uint32_t AlignTo(std::uint32_t size, std::uint32_t alignment) noexcept(true)
	{
		return size + ((alignment - (size % alignment)) % alignment);
	}

	//////////////////////////////////////////////////////////////////////////

	/** Copy data from a host visible buffer to a device local buffer using the specified command pool */
	inline void CopyHostVisibleBufferToDeviceLocalBuffer(
		const VulkanDevice& device,
		const VulkanCommandPool& command_pool,
		const memory::VulkanBuffer& host_visible_buffer,
		const memory::VulkanBuffer& device_local_buffer) noexcept(true)
	{
		VulkanCommandBuffer cmd_buffer = {};
		cmd_buffer.Create(device, command_pool, 1);
		cmd_buffer.BeginRecording(CommandBufferUsage::OneTimeSubmit);

		VkBufferCopy region = {};
		region.srcOffset = host_visible_buffer.info.offset;
		region.size = host_visible_buffer.info.size;
		region.dstOffset = device_local_buffer.info.offset;

		// Copy command
		vkCmdCopyBuffer(cmd_buffer.GetNative(), host_visible_buffer.buffer, device_local_buffer.buffer, 1, &region);

		// Get hold of the graphics queue since that one is guaranteed to be able to perform transfers as well
		auto& queue = device.GetQueueNativeOfType(VulkanQueueType::Graphics);

		// Execute the command
		cmd_buffer.StopRecording();
		cmd_buffer.Submit(queue);
		vkQueueWaitIdle(queue);

		cmd_buffer.Destroy(device, command_pool);
	}

	//////////////////////////////////////////////////////////////////////////
}

#endif
