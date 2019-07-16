#ifndef VULKAN_ENUMS_HPP
#define VULKAN_ENUMS_HPP

// Vulkan
#include <vulkan/vulkan.h>

namespace vkc::vk_wrapper::enums
{
	/** Queue types supported by this application */
	enum class VulkanQueueType
	{
		Graphics,
		Present,
		Compute
	};

	/** Supported vertex topology types */
	enum class VertexTopologyType
	{
		TriangleFan = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN,
		TriangleList = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		TriangleStrip = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,

		LineList = VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
		LineListAdjacent = VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY,

		LineStrip = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP,
		LineStripAdjacent = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY,

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
		Clockwise = VK_FRONT_FACE_CLOCKWISE,
		CounterClockwise = VK_FRONT_FACE_COUNTER_CLOCKWISE,
	};
}

#endif
