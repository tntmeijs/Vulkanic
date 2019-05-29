#ifndef VIRTUAL_BUFFER_HPP
#define VIRTUAL_BUFFER_HPP

// Vulkan
#include <vulkan/vulkan.h>

// C++ standard
#include <cstdint>

namespace vkc::memory
{
	class MemoryBlock;

	/** Class that can be used by the user to access GPU memory */
	class VirtualBuffer
	{
	public:
		VirtualBuffer(
			const VkBuffer& buffer_ref,
			const VkDeviceMemory& memory_ref,
			MemoryBlock* const parent_block_ptr,
			std::uint32_t id) noexcept(true);

		~VirtualBuffer() noexcept(true) {}

		/** Map a region of the memory block to the CPU-visible pointer */
		void Map(const VkDevice& device) noexcept(true);

		/** Unmap a region of the memory block from the CPU-visible pointer */
		void UnMap(const VkDevice& device) const noexcept(true);

		/** Set the size of the buffer */
		void SetSize(std::uint32_t size) noexcept(true);

		/** Set the offset of the buffer */
		void SetOffset(std::uint32_t offset) noexcept(true);

		/** Get the size of the buffer */
		const std::uint32_t Size() const noexcept(true);

		/** Get the offset of this buffer */
		const std::uint32_t Offset() const noexcept(true);

		/** Get a constant pointer to the data that backs this buffer */
		void* const Data() const noexcept(true);

		/** Get a reference to the Vulkan buffer */
		const VkBuffer& Buffer() const noexcept(true);

		/** Get a reference to the Vulkan buffer memory */
		const VkDeviceMemory& Memory() const noexcept(true);

		/** Request the memory block to destroy this virtual buffer */
		void Deallocate() const noexcept(false);

	private:
		const VkBuffer& m_buffer;
		const VkDeviceMemory& m_memory;
		MemoryBlock* const m_parent_block;
		std::uint32_t m_offset;
		std::uint32_t m_size;
		std::uint32_t m_id;
		void* m_data;
	};
}

#endif
