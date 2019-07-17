// Application
#include "vertex.hpp"

using namespace vkc;

const std::vector<VkVertexInputBindingDescription>& VertexBase::GetBindingDescriptions() const noexcept(true)
{
	return m_vertex_binding_descriptions;
}

const std::vector<VkVertexInputAttributeDescription>& VertexBase::GetAttributeDescriptions() const noexcept(true)
{
	return m_vertex_attribute_descriptions;
}

VertexPCT::VertexPCT() noexcept(true)
	: position({})
	, color({})
	, texture_coordinate({})
{
	//////////////////////////////////////////////////////////////////////////
	// Set-up vertex input binding descriptions
	//////////////////////////////////////////////////////////////////////////
	VkVertexInputBindingDescription binding_desc = {};
	binding_desc.binding = 0;
	binding_desc.stride = sizeof(VertexPCT);
	binding_desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	m_vertex_binding_descriptions.push_back(binding_desc);

	//////////////////////////////////////////////////////////////////////////
	// Set-up vertex input attribute descriptions
	//////////////////////////////////////////////////////////////////////////
	VkVertexInputAttributeDescription position_attrib = {};
	position_attrib.binding = 0;
	position_attrib.location = 0;
	position_attrib.format = VK_FORMAT_R32G32B32_SFLOAT;
	position_attrib.offset = offsetof(VertexPCT, position);
	m_vertex_attribute_descriptions.push_back(position_attrib);

	VkVertexInputAttributeDescription color_attrib = {};
	color_attrib.binding = 0;
	color_attrib.location = 1;
	color_attrib.format = VK_FORMAT_R32G32B32_SFLOAT;
	color_attrib.offset = offsetof(VertexPCT, color);
	m_vertex_attribute_descriptions.push_back(color_attrib);

	VkVertexInputAttributeDescription texcoord_attrib = {};
	texcoord_attrib.binding = 0;
	texcoord_attrib.location = 2;
	texcoord_attrib.format = VK_FORMAT_R32G32_SFLOAT;
	texcoord_attrib.offset = offsetof(VertexPCT, texture_coordinate);
	m_vertex_attribute_descriptions.push_back(texcoord_attrib);
}

VertexPCT::~VertexPCT() noexcept(true)
{}
