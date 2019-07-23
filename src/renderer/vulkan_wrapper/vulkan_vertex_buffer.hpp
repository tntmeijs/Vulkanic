#ifndef VULKAN_VERTEX_BUFFER_HPP
#define VULKAN_VERTEX_BUFFER_HPP

// Application
#include "renderer/memory_manager/memory_manager.hpp"
#include "vulkan_command_pool.hpp"
#include "vulkan_device.hpp"
#include "vulkan_functions.hpp"

// Vulkan
#include <vulkan/vulkan.h>

// C++ standard
#include <vector>

namespace vkc::vk_wrapper
{
	/** Wrapper class that abstracts vertex buffer creation */
	class VulkanVertexBuffer
	{
	public:
		VulkanVertexBuffer() noexcept(true);
		~VulkanVertexBuffer() noexcept(true);

		/** Create a new vertex buffer using the specified vertex data */
		template<class VERTEX>
		void Create(
			const VulkanDevice& device,
			const VulkanCommandPool& command_pool,
			const std::vector<VERTEX>& vertices) noexcept(true);

		/** Free the allocated vertex buffer memory */
		void Destroy() const noexcept(true);

		/** Get a reference to the underlaying Vulkan buffer object */
		const VkBuffer& GetNative() const noexcept(true);

	private:
		memory::VulkanBuffer m_vertex_buffer;
	};

	template<class VERTEX>
	inline void VulkanVertexBuffer::Create(
		const VulkanDevice& device,
		const VulkanCommandPool& command_pool,
		const std::vector<VERTEX>& vertices) noexcept(true)
	{
		VkDeviceSize buffer_size = sizeof(VERTEX) * vertices.size();

		// Create a staging buffer
		memory::BufferAllocationInfo staging_buffer_alloc_info = {};
		staging_buffer_alloc_info.buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		staging_buffer_alloc_info.buffer_create_info.size = buffer_size;
		staging_buffer_alloc_info.buffer_create_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		staging_buffer_alloc_info.buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		staging_buffer_alloc_info.allocation_info.usage = VMA_MEMORY_USAGE_CPU_ONLY;
		staging_buffer_alloc_info.allocation_info.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

		auto staging_buffer = memory::MemoryManager::GetInstance().Allocate(staging_buffer_alloc_info);

		// Copy the vertex data to the staging buffer
		memcpy(staging_buffer.info.pMappedData, vertices.data(), static_cast<size_t>(buffer_size));

		// Create a GPU-visible vertex buffer
		memory::BufferAllocationInfo vertex_buffer_alloc_info = {};
		vertex_buffer_alloc_info.buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		vertex_buffer_alloc_info.buffer_create_info.size = buffer_size;
		vertex_buffer_alloc_info.buffer_create_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		vertex_buffer_alloc_info.buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		vertex_buffer_alloc_info.allocation_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;

		m_vertex_buffer = memory::MemoryManager::GetInstance().Allocate(vertex_buffer_alloc_info);

		// Copy the staging buffer to device local memory
		func::CopyHostVisibleBufferToDeviceLocalBuffer(device, command_pool, staging_buffer, m_vertex_buffer);

		// No need to keep the staging buffer around anymore
		memory::MemoryManager::GetInstance().Free(staging_buffer);
	}
}

#endif // VULKAN_VERTEX_BUFFER_HPP
