#ifndef MEMORY_MANAGER_HPP
#define MEMORY_MANAGER_HPP

// Vulkan
#include <vulkan/vulkan.h>

// C++ standard
#include <memory>
#include <vector>

namespace vkc::memory
{
	class VirtualBuffer;
	class MemoryChunk;

	class MemoryManager
	{
	public:
		MemoryManager(std::uint32_t size_per_block) noexcept(true);
		~MemoryManager() noexcept(true);

		/** Request a new allocation */
		/**
		 * Allows the application to request a new block of memory. If there is
		 * enough space in an existing block, that block will be returned.
		 *
		 * If the total requested size is larger than the hard-coded maximum
		 * block size, the memory manager will allocate a new block specifically
		 * for this object.
		 *
		 * If the requested memory does not fit in an existing block, the memory
		 * manager will allocate a new block of a fixed size and use that to
		 * store the resource in.
		 */
		const VirtualBuffer& AllocateBuffer(
			const VkDevice& device,
			const VkPhysicalDevice& physical_device,
			VkDeviceSize size,
			VkBufferUsageFlags buffer_usage,
			VkMemoryPropertyFlags memory_properties) noexcept(false);

		/** Clean-up all buffers */
		void Destroy() noexcept(true);

	private:
		std::uint32_t m_size_per_block;
		std::vector<std::unique_ptr<MemoryChunk>> m_chunks;
	};
}

#endif
