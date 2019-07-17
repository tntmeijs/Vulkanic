// Application
#include "vertex.hpp"

using namespace vkc;

VertexPCT::VertexPCT() noexcept(true)
	: position({})
	, color({})
	, texture_coordinate({})
{}

VertexPCT::VertexPCT(const glm::vec3& position, const glm::vec3& color, const glm::vec2& texture_coordinate)
	: position(position)
	, color(color)
	, texture_coordinate(texture_coordinate)
{}

VertexPCT::~VertexPCT() noexcept(true)
{}

std::vector<VkVertexInputBindingDescription> vkc::VertexPCT::GetBindingDescriptions() noexcept(true)
{
	VkVertexInputBindingDescription binding_desc = {};
	binding_desc.binding = 0;
	binding_desc.stride = sizeof(VertexPCT);
	binding_desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	return { binding_desc };
}

std::vector<VkVertexInputAttributeDescription> vkc::VertexPCT::GetAttributeDescriptions() noexcept(true)
{
	VkVertexInputAttributeDescription position_attrib = {};
	position_attrib.binding = 0;
	position_attrib.location = 0;
	position_attrib.format = VK_FORMAT_R32G32B32_SFLOAT;
	position_attrib.offset = offsetof(VertexPCT, position);

	VkVertexInputAttributeDescription color_attrib = {};
	color_attrib.binding = 0;
	color_attrib.location = 1;
	color_attrib.format = VK_FORMAT_R32G32B32_SFLOAT;
	color_attrib.offset = offsetof(VertexPCT, color);

	VkVertexInputAttributeDescription texture_coordinate_attrib = {};
	texture_coordinate_attrib.binding = 0;
	texture_coordinate_attrib.location = 2;
	texture_coordinate_attrib.format = VK_FORMAT_R32G32_SFLOAT;
	texture_coordinate_attrib.offset = offsetof(VertexPCT, texture_coordinate);

	return { position_attrib, color_attrib, texture_coordinate_attrib };
}
