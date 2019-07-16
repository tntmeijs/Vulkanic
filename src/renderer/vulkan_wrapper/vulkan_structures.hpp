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
	/** Base class for a Vulkan pipeline information structure */
	struct VulkanPipelineInfo
	{
		virtual ~VulkanPipelineInfo() {}
	};
	
	/** All information to create a graphics pipeline */
	// #TODO: Refactor this structure and split it up into multiple structures
	// #TODO: Add multisampling configuration
	// #TODO: Add color blending configuration
	struct VulkanGraphicsPipelineInfo : public VulkanPipelineInfo
	{
		// Vertex data information
		std::vector<VkVertexInputBindingDescription> vertex_binding_descs;
		std::vector<VkVertexInputAttributeDescription> vertex_attribute_descs;
		enums::VertexTopologyType topology;
		
		// Viewport and scissor rect
		VkViewport viewport;
		VkRect2D scissor_rect;

		// Rasterization state
		bool enable_depth_clamping;
		bool discard_rasterizer_output;
		enums::PolygonFillMode polygon_fill_mode;
		float line_width;
		enums::PolygonFaceCullMode cull_mode;
		enums::TriangleWindingOrder winding_order;
		bool enable_depth_bias;
	};
	
	/** All information to create a compute pipeline */
	struct VulkanComputePipelineInfo : public VulkanPipelineInfo
	{
	};
	
	/** All information to create a ray-tracing pipeline */
	struct VulkanRayTracingPipelineInfo : public VulkanPipelineInfo
	{
	};

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
