// Application
#include "memory_block.hpp"
#include "miscellaneous/exceptions.hpp"
#include "renderer/vulkan_wrapper/vulkan_functions.hpp"
#include "virtual_buffer.hpp"

using namespace vkc::exception;
using namespace vkc::memory;
using namespace vkc::vk_wrapper::func;

MemoryBlock::MemoryBlock(
	const VkDevice& device,
	const VkPhysicalDevice& physical_device,
	VkBufferUsageFlags usage,
	VkMemoryPropertyFlags properties,
	std::uint32_t max_block_size) noexcept(false)
	: m_max_block_size(max_block_size)
	, m_current_size(0)
	, m_end(0)
	, m_buffer(VK_NULL_HANDLE)
	, m_memory(VK_NULL_HANDLE)
	, m_device(device)
	, m_physical_device(physical_device)
{
	VkBufferCreateInfo buffer_info = {};
	buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_info.size = m_max_block_size;
	buffer_info.usage = usage;
	buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	// Create the buffer
	auto result = vkCreateBuffer(device, &buffer_info, nullptr, &m_buffer);

	if (result != VK_SUCCESS)
	{
		throw CriticalVulkanError("Could not create a buffer.");
	}

	// Query memory requirements
	VkMemoryRequirements memory_requirements = {};
	vkGetBufferMemoryRequirements(device, m_buffer, &memory_requirements);

	// Required alignment
	m_alignment = memory_requirements.alignment;

	VkMemoryAllocateInfo allocation_info = {};
	allocation_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocation_info.allocationSize = memory_requirements.size;
	allocation_info.memoryTypeIndex = FindMemoryTypeIndex(
		memory_requirements.memoryTypeBits,
		properties,
		physical_device);

	// Allocate GPU memory
	result = vkAllocateMemory(device, &allocation_info, nullptr, &m_memory);

	if (result != VK_SUCCESS)
	{
		throw CriticalVulkanError("Could not allocate buffer memory.");
	}

	// Associate the GPU memory with the buffer
	vkBindBufferMemory(device, m_buffer, m_memory, 0);
}

MemoryBlock::~MemoryBlock() noexcept(true)
{
	// Get rid of all virtual buffers
	m_virtual_buffers.clear();

	// Deallocate the Vulkan buffer
	vkFreeMemory(m_device, m_memory, nullptr);
	vkDestroyBuffer(m_device, m_buffer, nullptr);
}

const VirtualBuffer& MemoryBlock::SubAllocate(
	VkDeviceSize size) noexcept(false)
{
	// Memory manager made a big mistake, this block cannot even store more data
	if (m_max_block_size - m_current_size < size)
	{
		throw GPUOutOfMemoryError("Cannot sub-allocate, block is out of memory.");
	}

	// Create a new virtual buffer (id is just the index into the vector before pushing back the new virtual buffer)
	m_virtual_buffers.push_back(std::make_unique<VirtualBuffer>(m_buffer, m_memory, this, static_cast<std::uint32_t>(m_virtual_buffers.size())));

	// Just pushed back the new virtual buffer, so the index is std::vector::size() - 1
	auto buffer_index = m_virtual_buffers.size() - 1;

	// Size of the virtual buffer within this memory block
	m_virtual_buffers[buffer_index]->SetSize(static_cast<std::uint32_t>(size));

	// Increase the total number of bytes stored in the buffer
	m_current_size += m_virtual_buffers[buffer_index]->Size();

	// Try to place the buffer at the end of the memory block
	if ((m_max_block_size - AlignTo(m_end, static_cast<std::uint32_t>(m_alignment))) >= size)
	{
		// Place the buffer at the end of the GPU memory block
		m_virtual_buffers[buffer_index]->SetOffset(AlignTo(m_end, static_cast<std::uint32_t>(m_alignment)));

		// The memory block has a new end because of the new virtual buffer
		m_end = m_virtual_buffers[buffer_index]->Offset() + m_virtual_buffers[buffer_index]->Size();

		// Done sub-allocating, return newly created virtual buffer
		return *m_virtual_buffers[m_virtual_buffers.size() - 1];
	}

	// Try to place the buffer somewhere in between existing virtual buffers
	for (auto index = 0; index < m_virtual_buffers.size() - 1; ++index)
	{
		auto& current_buffer = m_virtual_buffers[index];
		auto& next_buffer = m_virtual_buffers[index + 1];

		auto current_buffer_end = current_buffer->Offset() + current_buffer->Size();
		auto next_buffer_start = next_buffer->Offset() + next_buffer->Size();

		// Align the end of the current buffer
		current_buffer_end = AlignTo(current_buffer_end, static_cast<std::uint32_t>(m_alignment));

		auto space_remaining = next_buffer_start - current_buffer_end;

		if (space_remaining >= size)
		{
			// Buffer fits in between existing buffers
			m_virtual_buffers[buffer_index]->SetOffset(current_buffer_end);

			// Done sub-allocating, return newly created virtual buffer
			return *m_virtual_buffers[buffer_index];
		}
	}

	// Could not perform any sub-allocation on this block
	throw GPUOutOfMemoryError("Could not perform any sub-allocation.");
}

const bool MemoryBlock::CanFit(
	VkDeviceSize size) const noexcept(true)
{
	// Does this virtual buffer fit at the end of the memory block?
	if ((m_max_block_size - AlignTo(m_end, static_cast<std::uint32_t>(m_alignment))) >= size)
	{
		return true;
	}

	// Does this virtual buffer fit somewhere in between existing virtual buffers?
	for (auto index = 0; index < m_virtual_buffers.size() - 1; ++index)
	{
		auto& current_buffer = m_virtual_buffers[index];
		auto& next_buffer = m_virtual_buffers[index + 1];

		auto current_buffer_end = current_buffer->Offset() + current_buffer->Size();
		auto next_buffer_start = next_buffer->Offset() + next_buffer->Size();

		auto space_remaining = next_buffer_start - current_buffer_end;

		if (space_remaining >= size)
		{
			// Buffer fits somewhere in between existing buffers
			return true;
		}
	}

	// Buffer does not fit
	return false;
}

void vkc::memory::MemoryBlock::DeallocateVirtualBuffer(std::uint32_t buffer_index) noexcept(false)
{
	m_virtual_buffers.erase(m_virtual_buffers.begin() + buffer_index);
}
