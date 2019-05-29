#ifndef MEMORY_CHUNK_HPP
#define MEMORY_CHUNK_HPP

// Vulkan
#include <vulkan/vulkan.h>

// C++ standard
#include <memory>
#include <vector>

namespace vkc::memory
{
	class MemoryBlock;
	class VirtualBuffer;

    /** Container used to store usage, properties, and the memory block */
	class MemoryChunk
	{
	public:
		MemoryChunk(
			VkBufferUsageFlags buffer_usage,
			VkMemoryPropertyFlags memory_properties) noexcept(true);

		~MemoryChunk() noexcept(true);

		/** Allocate a buffer in this chunk */
		const VirtualBuffer& AllocateBufferInChunk(
			const VkDevice& device,
			const VkPhysicalDevice& physical_device,
			std::uint32_t max_block_size,
			VkDeviceSize size,
			VkBufferUsageFlags buffer_usage,
			VkMemoryPropertyFlags memory_properties) noexcept(false);

		/** Get the specified buffer usage for this chunk */
		const VkBufferUsageFlags BufferUsage() const noexcept(true);

		/** Get the specified memory properties for this chunk */
		const VkMemoryPropertyFlags MemoryProperties() const noexcept(true);

	private:
		VkBufferUsageFlags m_buffer_usage;
		VkMemoryPropertyFlags m_memory_properties;
		std::vector<std::unique_ptr<MemoryBlock>> m_memory_blocks;
	};
}

#endif
