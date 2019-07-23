// Application
#include "vulkan_index_buffer.hpp"

using namespace vkc::memory;
using namespace vkc::vk_wrapper;

VulkanIndexBuffer::VulkanIndexBuffer() noexcept(true)
	: m_index_buffer({})
{}

VulkanIndexBuffer::~VulkanIndexBuffer() noexcept(true)
{}

void VulkanIndexBuffer::Destroy() const noexcept(true)
{
	MemoryManager::GetInstance().Free(m_index_buffer);
}

const VkBuffer& VulkanIndexBuffer::GetNative() const noexcept(true)
{
	return m_index_buffer.buffer;
}
