#ifndef MEMORY_BLOCK_HPP
#define MEMORY_BLOCK_HPP

// Vulkan
#include <vulkan/vulkan.h>

// C++ standard
#include <memory>
#include <vector>

namespace vkc::memory
{
	class VirtualBuffer;

	/** Internal class used to allocate big blocks of memory */
	/**
	 * Virtual buffers are used to split up the memory allocated in a block.
	 * Each block uses a hard-coded size of 64MB. This value is just an
	 * arbitrary value, but it works well.
	 */
	class MemoryBlock
	{
	public:
		/** Allocate a new block of GPU memory */
		MemoryBlock(
			const VkDevice& device,
			const VkPhysicalDevice& physical_device,
			VkBufferUsageFlags usage,
			VkMemoryPropertyFlags properties,
			std::uint32_t max_block_size) noexcept(false);

		~MemoryBlock() noexcept(true);

		/** Allocate a new virtual buffer in this block */
		const VirtualBuffer& SubAllocate(
			VkDeviceSize size) noexcept(false);

		/** Check if this block has enough space left to store the buffer */
		const bool CanFit(VkDeviceSize size) const noexcept(true);

		/** Deallocate a virtual buffer */
		void DeallocateVirtualBuffer(std::uint32_t buffer_index) noexcept(false);

	private:
		std::uint32_t m_max_block_size;
		std::uint32_t m_current_size;
		std::uint32_t m_end;

		const VkDevice& m_device;
		const VkPhysicalDevice& m_physical_device;

		VkBuffer m_buffer;
		VkDeviceMemory m_memory;
		VkDeviceSize m_alignment;

		std::vector<std::unique_ptr<VirtualBuffer>> m_virtual_buffers;
	};
}

#endif
