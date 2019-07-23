// Application
#include "vulkan_vertex_buffer.hpp"

using namespace vkc::memory;
using namespace vkc::vk_wrapper;

VulkanVertexBuffer::VulkanVertexBuffer() noexcept(true)
	: m_vertex_buffer({})
{}

VulkanVertexBuffer::~VulkanVertexBuffer() noexcept(true)
{}

void VulkanVertexBuffer::Destroy() const noexcept(true)
{
	MemoryManager::GetInstance().Free(m_vertex_buffer);
}

const VkBuffer& VulkanVertexBuffer::GetNative() const noexcept(true)
{
	return m_vertex_buffer.buffer;
}
