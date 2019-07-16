#ifndef VULKAN_PIPELINE_INFO_HPP
#define VULKAN_PIPELINE_INFO_HPP

// Vulkan
#include <vulkan/vulkan.h>

// C++ standard
#include <vector>

namespace vkc::vk_wrapper
{
	/** Supported vertex topology types */
	enum class VertexTopologyType
	{
		TriangleFan		= VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN,
		TriangleList	= VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		TriangleStrip	= VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,

		LineList			= VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
		LineListAdjacent	= VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY,

		LineStrip			= VK_PRIMITIVE_TOPOLOGY_LINE_STRIP,
		LineStripAdjacent	= VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY,

		PointList = VK_PRIMITIVE_TOPOLOGY_POINT_LIST
	};

	/** Supported polygon fill modes */
	enum class PolygonFillMode
	{
		Fill	= VK_POLYGON_MODE_FILL,
		Line	= VK_POLYGON_MODE_LINE,
		Point	= VK_POLYGON_MODE_POINT,
	};

	/** Supported cull modes */
	enum class PolygonFaceCullMode
	{
		None				= VK_CULL_MODE_NONE,
		FrontFace			= VK_CULL_MODE_FRONT_BIT,
		BackFace			= VK_CULL_MODE_BACK_BIT,
		FrontAndBackFaces	= VK_CULL_MODE_FRONT_AND_BACK
	};

	/** Triangle winding order */
	enum class TriangleWindingOrder
	{
		Clockwise			= VK_FRONT_FACE_CLOCKWISE,
		CounterClockwise	= VK_FRONT_FACE_COUNTER_CLOCKWISE,
	};

	/** Base class for a Vulkan pipeline information structure */
	struct VulkanPipelineInfo
	{
		virtual ~VulkanPipelineInfo() {}
	};

	/** All information to create a graphics pipeline */
	/** #TODO: Refactor this structure and split it up into multiple structures */
	/** #TODO: Add multi sampling configuration */
	/** #TODO: Add color blending configuration */
	struct VulkanGraphicsPipelineInfo : public VulkanPipelineInfo
	{
		// Vertex data information
		std::vector<VkVertexInputBindingDescription> vertex_binding_descs;
		std::vector<VkVertexInputAttributeDescription> vertex_attribute_descs;
		VertexTopologyType topology;

		// Viewport and scissor rect
		VkViewport viewport;
		VkRect2D scissor_rect;

		// Rasterization state
		bool enable_depth_clamping;
		bool discard_rasterizer_output;
		PolygonFillMode polygon_fill_mode;
		float line_width;
		PolygonFaceCullMode cull_mode;
		TriangleWindingOrder winding_order;
		bool enable_depth_bias;
	};

	/** All information to create a compute pipeline */
	struct VulkanComputePipelineInfo : public VulkanPipelineInfo
	{};

	/** All information to create a ray-tracing pipeline */
	struct VulkanRayTracingPipelineInfo : public VulkanPipelineInfo
	{};
}

#endif // VULKAN_PIPELINE_INFO_HPP
