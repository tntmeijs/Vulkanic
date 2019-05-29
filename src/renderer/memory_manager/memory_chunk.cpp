// Application
#include "memory_block.hpp"
#include "memory_chunk.hpp"

using namespace vkc::memory;

MemoryChunk::MemoryChunk(
	VkBufferUsageFlags buffer_usage,
	VkMemoryPropertyFlags memory_properties) noexcept(true)
	: m_buffer_usage(buffer_usage)
	, m_memory_properties(memory_properties)
{}

MemoryChunk::~MemoryChunk() noexcept(true)
{}

const VirtualBuffer& MemoryChunk::AllocateBufferInChunk(
	const VkDevice& device,
	const VkPhysicalDevice& physical_device,
	std::uint32_t max_block_size,
	VkDeviceSize size,
	VkBufferUsageFlags buffer_usage,
	VkMemoryPropertyFlags memory_properties) noexcept(false)
{
	// Find the first memory block that can store this buffer
	for (auto& block : m_memory_blocks)
	{
		// Does this block have enough space left?
		if (block->CanFit(size))
		{
			// Enough space left, allocate this block
			return block->SubAllocate(size);
		}
	}

	// No block with enough memory found, allocate a new one
	m_memory_blocks.push_back(std::make_unique<MemoryBlock>(
		device,
		physical_device,
		buffer_usage,
		memory_properties,
		max_block_size));

	// Retry buffer allocation
	return AllocateBufferInChunk(
		device,
		physical_device,
		max_block_size,
		size,
		buffer_usage,
		memory_properties);
}

const VkBufferUsageFlags MemoryChunk::BufferUsage() const noexcept(true)
{
	return m_buffer_usage;
}

const VkMemoryPropertyFlags MemoryChunk::MemoryProperties() const noexcept(true)
{
	return m_memory_properties;
}
