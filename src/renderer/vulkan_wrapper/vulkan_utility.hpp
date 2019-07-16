#ifndef VK_UTILITY_HPP
#define VK_UTILITY_HPP

// Application
#include "miscellaneous/exceptions.hpp"
#include "vulkan_command_buffer.hpp"
#include "vulkan_command_pool.hpp"
#include "vulkan_device.hpp"

// C++ standard
#include <string>
#include <vector>

namespace vkc::vk_wrapper::utility
{
	/** Convert a vector of std::string to a vector of const char* */
	/**
	 * Vulkan still uses a C API under the hood, so sometimes it is required to
	 * pass an array of c-strings. This function makes the conversion easy.
	 */
	inline const std::vector<const char*> ConvertVectorOfStringsToCString(
		const std::vector<std::string>& original) noexcept(true)
	{
		std::vector<const char*> cstring_vector;

		for (const auto& str : original)
		{
			cstring_vector.push_back(str.c_str());
		}

		return cstring_vector;
	}

	/** Check if the required names exist in the "list" of all names */
	inline bool AllRequiredItemsExistInVector(
		const std::vector<std::string>& required_names,
		const std::vector<std::string>& all_names) noexcept(true)
	{
		if (all_names.empty())
		{
			// No required names available
			return false;
		}
		else if (required_names.empty())
		{
			// No required names given
			return true;
		}

		// Check if all requested names are available
		for (const auto& required_name : required_names)
		{
			bool found_required_name = false;

			for (const auto& name : all_names)
			{
				if (required_name == name)
				{
					found_required_name = true;
				}
			}

			// Could not find the specified name
			if (!found_required_name)
			{
				return false;
			}
		}

		return true;
	}

	/** Transition an image layout from the current layout to a new layout */
	inline void TransitionImageLayout(
		const VulkanDevice& device,
		const VulkanCommandPool& command_pool,
		const VkImage& image,
		VkImageLayout current_layout,
		VkImageLayout new_layout) noexcept(false)
	{
		VulkanCommandBuffer cmd_buffer = {};
		cmd_buffer.Create(device, command_pool, 1);
		cmd_buffer.BeginRecording(CommandBufferUsage::OneTimeSubmit);

		VkPipelineStageFlags source_stage = {}, destination_stage = {};

		VkImageMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = current_layout;
		barrier.newLayout = new_layout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = image;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.layerCount = 1;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.baseMipLevel = 0;

		if (current_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
		{
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (current_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else
		{
			throw exception::CriticalVulkanError("Unsupported image layout transition.");
		}

		// Record the transition commands
		vkCmdPipelineBarrier(
			cmd_buffer.GetNative(),
			source_stage,
			destination_stage,
			0,	// Flags
			0,	// No memory barriers
			nullptr,
			0,	// No buffer memory barriers
			nullptr,
			1,	// One image barrier
			&barrier);

		auto graphics_queue = device.GetQueueNativeOfType(VulkanQueueType::Graphics);
		
		// Execute the commands on the graphics queue
		cmd_buffer.StopRecording();
		cmd_buffer.Submit(graphics_queue);
		vkQueueWaitIdle(graphics_queue);

		// Command buffer is no longer needed
		cmd_buffer.Destroy(device, command_pool);
	}

	/** Get the number of bits per channel from a VkFormat (some uncommon formats have been excluded, invalid format == 0) */
	inline std::uint32_t VulkanFormatToBitsPerChannel(VkFormat format) noexcept(true)
	{
		switch (format)
		{
			// 8 bits per channel
			case VK_FORMAT_R8_UNORM:
			case VK_FORMAT_R8_SNORM:
			case VK_FORMAT_R8_USCALED:
			case VK_FORMAT_R8_SSCALED:
			case VK_FORMAT_R8_UINT:
			case VK_FORMAT_R8_SINT:
			case VK_FORMAT_R8_SRGB:
			case VK_FORMAT_R8G8_UNORM:
			case VK_FORMAT_R8G8_SNORM:
			case VK_FORMAT_R8G8_USCALED:
			case VK_FORMAT_R8G8_SSCALED:
			case VK_FORMAT_R8G8_UINT:
			case VK_FORMAT_R8G8_SINT:
			case VK_FORMAT_R8G8_SRGB:
			case VK_FORMAT_R8G8B8_UNORM:
			case VK_FORMAT_R8G8B8_SNORM:
			case VK_FORMAT_R8G8B8_USCALED:
			case VK_FORMAT_R8G8B8_SSCALED:
			case VK_FORMAT_R8G8B8_UINT:
			case VK_FORMAT_R8G8B8_SINT:
			case VK_FORMAT_R8G8B8_SRGB:
			case VK_FORMAT_B8G8R8_UNORM:
			case VK_FORMAT_B8G8R8_SNORM:
			case VK_FORMAT_B8G8R8_USCALED:
			case VK_FORMAT_B8G8R8_SSCALED:
			case VK_FORMAT_B8G8R8_UINT:
			case VK_FORMAT_B8G8R8_SINT:
			case VK_FORMAT_B8G8R8_SRGB:
			case VK_FORMAT_R8G8B8A8_UNORM:
			case VK_FORMAT_R8G8B8A8_SNORM:
			case VK_FORMAT_R8G8B8A8_USCALED:
			case VK_FORMAT_R8G8B8A8_SSCALED:
			case VK_FORMAT_R8G8B8A8_UINT:
			case VK_FORMAT_R8G8B8A8_SINT:
			case VK_FORMAT_R8G8B8A8_SRGB:
			case VK_FORMAT_B8G8R8A8_UNORM:
			case VK_FORMAT_B8G8R8A8_SNORM:
			case VK_FORMAT_B8G8R8A8_USCALED:
			case VK_FORMAT_B8G8R8A8_SSCALED:
			case VK_FORMAT_B8G8R8A8_UINT:
			case VK_FORMAT_B8G8R8A8_SINT:
			case VK_FORMAT_B8G8R8A8_SRGB:
			case VK_FORMAT_A8B8G8R8_UNORM_PACK32:
			case VK_FORMAT_A8B8G8R8_SNORM_PACK32:
			case VK_FORMAT_A8B8G8R8_USCALED_PACK32:
			case VK_FORMAT_A8B8G8R8_SSCALED_PACK32:
			case VK_FORMAT_A8B8G8R8_UINT_PACK32:
			case VK_FORMAT_A8B8G8R8_SINT_PACK32:
			case VK_FORMAT_A8B8G8R8_SRGB_PACK32:
			case VK_FORMAT_S8_UINT:
				return 8;
				break;

			// 16 bits per channel
			case VK_FORMAT_R16_UNORM:
			case VK_FORMAT_R16_SNORM:
			case VK_FORMAT_R16_USCALED:
			case VK_FORMAT_R16_SSCALED:
			case VK_FORMAT_R16_UINT:
			case VK_FORMAT_R16_SINT:
			case VK_FORMAT_R16_SFLOAT:
			case VK_FORMAT_R16G16_UNORM:
			case VK_FORMAT_R16G16_SNORM:
			case VK_FORMAT_R16G16_USCALED:
			case VK_FORMAT_R16G16_SSCALED:
			case VK_FORMAT_R16G16_UINT:
			case VK_FORMAT_R16G16_SINT:
			case VK_FORMAT_R16G16_SFLOAT:
			case VK_FORMAT_R16G16B16_UNORM:
			case VK_FORMAT_R16G16B16_SNORM:
			case VK_FORMAT_R16G16B16_USCALED:
			case VK_FORMAT_R16G16B16_SSCALED:
			case VK_FORMAT_R16G16B16_UINT:
			case VK_FORMAT_R16G16B16_SINT:
			case VK_FORMAT_R16G16B16_SFLOAT:
			case VK_FORMAT_R16G16B16A16_UNORM:
			case VK_FORMAT_R16G16B16A16_SNORM:
			case VK_FORMAT_R16G16B16A16_USCALED:
			case VK_FORMAT_R16G16B16A16_SSCALED:
			case VK_FORMAT_R16G16B16A16_UINT:
			case VK_FORMAT_R16G16B16A16_SINT:
			case VK_FORMAT_R16G16B16A16_SFLOAT:
			case VK_FORMAT_D16_UNORM:
				return 16;
				break;

			// 24 bits per channel
			case VK_FORMAT_X8_D24_UNORM_PACK32:
				return 24;
				break;

			// 32 bits per channel
			case VK_FORMAT_R32_UINT:
			case VK_FORMAT_R32_SINT:
			case VK_FORMAT_R32_SFLOAT:
			case VK_FORMAT_R32G32_UINT:
			case VK_FORMAT_R32G32_SINT:
			case VK_FORMAT_R32G32_SFLOAT:
			case VK_FORMAT_R32G32B32_UINT:
			case VK_FORMAT_R32G32B32_SINT:
			case VK_FORMAT_R32G32B32_SFLOAT:
			case VK_FORMAT_R32G32B32A32_UINT:
			case VK_FORMAT_R32G32B32A32_SINT:
			case VK_FORMAT_R32G32B32A32_SFLOAT:
			case VK_FORMAT_D32_SFLOAT:
				return 32;
				break;

			// 64 bits per channel
			case VK_FORMAT_R64_UINT:
			case VK_FORMAT_R64_SINT:
			case VK_FORMAT_R64_SFLOAT:
			case VK_FORMAT_R64G64_UINT:
			case VK_FORMAT_R64G64_SINT:
			case VK_FORMAT_R64G64_SFLOAT:
			case VK_FORMAT_R64G64B64_UINT:
			case VK_FORMAT_R64G64B64_SINT:
			case VK_FORMAT_R64G64B64_SFLOAT:
			case VK_FORMAT_R64G64B64A64_UINT:
			case VK_FORMAT_R64G64B64A64_SINT:
			case VK_FORMAT_R64G64B64A64_SFLOAT:
				return 64;
				break;

			// Invalid
			default:
				return 0;
				break;
		}
	}

	/** Get the number of bytes per channel from a VkFormat (some uncommon formats have been excluded, invalid format == 0) */
	inline std::uint32_t VulkanFormatToBytesPerChannel(VkFormat format) noexcept(true)
	{
		std::uint32_t bits_per_channel = VulkanFormatToBitsPerChannel(format);

		// Return 0 when invalid, else, return the value in bytes by dividing the bit count by 8
		return (bits_per_channel == 0) ? 0 : bits_per_channel / 8;
	}
}

#endif
