// Application
#include "memory_manager.hpp"
#include "renderer/memory_manager/memory_chunk.hpp"

using namespace vkc::memory;

MemoryManager::MemoryManager(std::uint32_t size_per_block) noexcept(true)
	: m_size_per_block(size_per_block)
{}

MemoryManager::~MemoryManager() noexcept(true)
{}

const VirtualBuffer& MemoryManager::AllocateBuffer(
	const VkDevice& device,
	const VkPhysicalDevice& physical_device,
	VkDeviceSize size,
	VkBufferUsageFlags buffer_usage,
	VkMemoryPropertyFlags memory_properties) noexcept(false)
{
	// Lambda for making it easy to find a correct chunk type
	auto chunk_suitable = [&buffer_usage, &memory_properties](
		const MemoryChunk* const chunk) {
		return (chunk->BufferUsage() == buffer_usage &&
				chunk->MemoryProperties() == memory_properties);
	};

	bool suitable_chunk_found = false;
	std::uint32_t suitable_chunk_index = 0;

	for (const auto& chunk : m_chunks)
	{
		// Find an existing memory chunk with the correct usage and properties
		if (chunk_suitable(chunk.get()))
		{
			// Found a correct chunk type
			suitable_chunk_found = true;
			break;
		}

		++suitable_chunk_index;
	}

	if (suitable_chunk_found)
	{
		// Allocate the buffer in the first suitable chunk
		return m_chunks[suitable_chunk_index]->AllocateBufferInChunk(
			device,
			physical_device,
			m_size_per_block,
			size,
			buffer_usage,
			memory_properties);
	}
	else
	{
		// Allocate a new chunk and retry
		m_chunks.push_back(std::make_unique<MemoryChunk>(buffer_usage, memory_properties));

		return AllocateBuffer(device, physical_device, size, buffer_usage, memory_properties);
	}
}

void vkc::memory::MemoryManager::Destroy() noexcept(true)
{
	// Get rid of all chunks
	m_chunks.clear();
}
