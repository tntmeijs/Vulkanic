#ifndef VULKAN_INDEX_BUFFER_HPP
#define VULKAN_INDEX_BUFFER_HPP

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
	/** Wrapper class that abstracts index buffer creation */
	class VulkanIndexBuffer
	{
	public:
		VulkanIndexBuffer() noexcept(true);
		~VulkanIndexBuffer() noexcept(true);

		/** Create a new index buffer using the specified index data */
		template<class INDEX>
		void Create(
			const VulkanDevice& device,
			const VulkanCommandPool& command_pool,
			const std::vector<INDEX>& vertices) noexcept(true);

		/** Free the allocated index buffer memory */
		void Destroy() const noexcept(true);

		/** Get a reference to the underlaying Vulkan buffer object */
		const VkBuffer& GetNative() const noexcept(true);

	private:
		memory::VulkanBuffer m_index_buffer;
	};

	template<class INDEX>
	inline void VulkanIndexBuffer::Create(
		const VulkanDevice& device,
		const VulkanCommandPool& command_pool,
		const std::vector<INDEX>& indices) noexcept(true)
	{
		VkDeviceSize buffer_size = sizeof(INDEX) * indices.size();

		// Create a staging buffer
		memory::BufferAllocationInfo staging_buffer_alloc_info = {};
		staging_buffer_alloc_info.buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		staging_buffer_alloc_info.buffer_create_info.size = buffer_size;
		staging_buffer_alloc_info.buffer_create_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		staging_buffer_alloc_info.buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		staging_buffer_alloc_info.allocation_info.usage = VMA_MEMORY_USAGE_CPU_ONLY;
		staging_buffer_alloc_info.allocation_info.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

		auto staging_buffer = memory::MemoryManager::GetInstance().Allocate(staging_buffer_alloc_info);

		// Copy the index data to the staging buffer
		memcpy(staging_buffer.info.pMappedData, indices.data(), static_cast<size_t>(buffer_size));

		// Create a GPU-visible index buffer
		memory::BufferAllocationInfo index_buffer_alloc_info = {};
		index_buffer_alloc_info.buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		index_buffer_alloc_info.buffer_create_info.size = buffer_size;
		index_buffer_alloc_info.buffer_create_info.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		index_buffer_alloc_info.buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		index_buffer_alloc_info.allocation_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;

		m_index_buffer = memory::MemoryManager::GetInstance().Allocate(index_buffer_alloc_info);

		// Copy the staging buffer to device local memory
		func::CopyHostVisibleBufferToDeviceLocalBuffer(device, command_pool, staging_buffer, m_index_buffer);

		// No need to keep the staging buffer around anymore
		memory::MemoryManager::GetInstance().Free(staging_buffer);
	}
}

#endif
