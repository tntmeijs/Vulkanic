#ifndef VULKAN_UNIFORM_BUFFER_HPP
#define VULKAN_UNIFORM_BUFFER_HPP

// Application
#include "renderer/memory_manager/memory_manager.hpp"

// Vulkan
#include <vulkan/vulkan.h>

namespace vkc::vk_wrapper
{
	class VulkanUniformBuffer
	{
	public:
		VulkanUniformBuffer() noexcept(true);
		~VulkanUniformBuffer() noexcept(true);

		/** Create a new uniform buffer for the specified data structure */
		template<class DATA>
		void Create() noexcept(true);

		/** Update the data in the uniform buffer (internally performs a memory map / unmap) */
		template<class DATA>
		void Update(const DATA& data) noexcept(true);

		/** Free the previously allocated uniform buffer */
		void Destroy() const noexcept(true);

		/** Get hold of the underlaying Vulkan buffer object */
		const VkBuffer& GetNative() const noexcept(true);

	private:
		memory::VulkanBuffer m_uniform_buffer;
	};

	template<class DATA>
	inline void VulkanUniformBuffer::Create() noexcept(true)
	{
		memory::BufferAllocationInfo uniform_buffer_alloc_info = {};
		uniform_buffer_alloc_info.buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		uniform_buffer_alloc_info.buffer_create_info.size = sizeof(DATA);
		uniform_buffer_alloc_info.buffer_create_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		uniform_buffer_alloc_info.buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		uniform_buffer_alloc_info.allocation_info.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

		m_uniform_buffer = memory::MemoryManager::GetInstance().Allocate(uniform_buffer_alloc_info);
	}

	template<class DATA>
	inline void VulkanUniformBuffer::Update(const DATA& data) noexcept(true)
	{
		auto* destination = memory::MemoryManager::GetInstance().MapBuffer(m_uniform_buffer);
		memcpy(destination, &data, sizeof(data));
		memory::MemoryManager::GetInstance().UnMapBuffer(m_uniform_buffer);
	}
}

#endif // VULKAN_UNIFORM_BUFFER_HPP
