#ifndef VULKAN_STRUCTURES_HPP
#define VULKAN_STRUCTURES_HPP

// Application
#include "vulkan_enums.hpp"

// Vulkan
#include <vulkan/vulkan.h>

// C++ standard
#include <optional>
#include <vector>

namespace vkc::vk_wrapper::structs
{
	/** Information to create a Vulkan render pass */
	// #TODO: Refactor
	struct VulkanRenderPassInfo
	{
		std::vector<VkAttachmentDescription> attachment_descriptions;
		std::vector<VkSubpassDescription> subpass_descriptions;
		std::vector<VkSubpassDependency> subpass_dependencies;
	};
}

#endif
