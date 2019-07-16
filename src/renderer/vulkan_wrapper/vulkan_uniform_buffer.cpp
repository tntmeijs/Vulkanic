// Application
#include "vulkan_uniform_buffer.hpp"

using namespace vkc::memory;
using namespace vkc::vk_wrapper;

VulkanUniformBuffer::VulkanUniformBuffer() noexcept(true)
	: m_uniform_buffer({})
{}

VulkanUniformBuffer::~VulkanUniformBuffer() noexcept(true)
{}

void VulkanUniformBuffer::Destroy() const noexcept(true)
{
	MemoryManager::GetInstance().Free(m_uniform_buffer);
}

const VkBuffer& VulkanUniformBuffer::GetNative() const noexcept(true)
{
	return m_uniform_buffer.buffer;
}
