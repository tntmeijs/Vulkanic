// Application
#include "memory_block.hpp"
#include "virtual_buffer.hpp"

using namespace uuids;
using namespace vkc::memory;

VirtualBuffer::VirtualBuffer(
	const VkBuffer& buffer_ref,
	const VkDeviceMemory& memory_ref,
	MemoryBlock* const parent_block_ptr,
	uuid uuid) noexcept(true)
	: m_buffer(buffer_ref)
	, m_memory(memory_ref)
	, m_parent_block(parent_block_ptr)
	, m_offset(0)
	, m_size(0)
	, m_data(nullptr)
	, m_uuid(uuid)
{}

VirtualBuffer::~VirtualBuffer() noexcept(true)
{}

void VirtualBuffer::Map(const VkDevice & device) noexcept(true)
{
	vkMapMemory(device, m_memory, m_offset, m_size, 0, &m_data);
}

void VirtualBuffer::UnMap(const VkDevice& device) const noexcept(true)
{
	vkUnmapMemory(device, m_memory);
}

void VirtualBuffer::SetSize(std::uint32_t size) noexcept(true)
{
	m_size = size;
}

void VirtualBuffer::SetOffset(std::uint32_t offset) noexcept(true)
{
	m_offset = offset;
}

const std::uint32_t VirtualBuffer::Size() const noexcept(true)
{
	return m_size;
}

const std::uint32_t VirtualBuffer::Offset() const noexcept(true)
{
	return m_offset;
}

void* const VirtualBuffer::Data() const noexcept(true)
{
	return m_data;
}

const VkBuffer& VirtualBuffer::Buffer() const noexcept(true)
{
	return m_buffer;
}

const VkDeviceMemory& VirtualBuffer::Memory() const noexcept(true)
{
	return m_memory;
}

void VirtualBuffer::Deallocate() const noexcept(false)
{
	m_parent_block->DeallocateVirtualBuffer(m_uuid);
}
