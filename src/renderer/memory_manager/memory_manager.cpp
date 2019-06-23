// Application
#include "memory_manager.hpp"
#include "miscellaneous/exceptions.hpp"
#include "renderer/vulkan_wrapper/vulkan_device.hpp"

// C++ standard
#include <algorithm>

using namespace vkc::exception;
using namespace vkc::memory;
using namespace vkc::vk_wrapper;

MemoryManager& MemoryManager::GetInstance()
{
	static MemoryManager instance;
	return instance;
}

void MemoryManager::Initialize(const VulkanDevice& device) noexcept(false)
{
	if (m_is_initialized)
	{
		// Memory manager already initialized, no need to create the allocator again
		return;
	}

	VmaAllocatorCreateInfo create_info = {};
	create_info.device = device.GetLogicalDeviceNative();
	create_info.physicalDevice = device.GetPhysicalDeviceNative();

	auto result = vmaCreateAllocator(&create_info, &m_allocator);

	if (result != VK_SUCCESS)
	{
		throw CriticalVulkanError("Failed to create an allocator.");
	}

	// Successfully initialized the memory manager
	m_is_initialized = true;
}

void MemoryManager::Destroy() noexcept(true)
{
	// Free any buffers that have not been freed yet
	for (const auto& buffer : m_buffers)
	{
		Free(buffer);
	}

	// Free any images that have not been freed yet
	for (const auto& image : m_images)
	{
		Free(image);
	}

	// Destroy the allocator (from here on, Initialize() has to be called once to make the memory manager work again)
	vmaDestroyAllocator(m_allocator);

	// Memory manager is no longer initialized
	m_is_initialized = false;
}

void MemoryManager::Free(const VulkanBuffer& buffer) noexcept(false)
{
	// Find the buffer with the same ID
	auto result = std::find_if(m_buffers.begin(), m_buffers.end(), [&buffer](const VulkanBuffer& other_buffer) {
		return (buffer.id == other_buffer.id);
	});

	// Could not find the buffer
	if (result == m_buffers.end())
	{
		throw CriticalVulkanError("Specified buffer cannot be freed, it does not exist.");
	}

	// Free the buffer
	vmaDestroyBuffer(m_allocator, buffer.buffer, buffer.allocation);

	// Remove the buffer from the container
	m_buffers.erase(result);
}

void MemoryManager::Free(const VulkanImage& image) noexcept(false)
{
	// Find the image with the same ID
	auto result = std::find_if(m_images.begin(), m_images.end(), [&image](const VulkanImage& other_image) {
		return (image.id == other_image.id);
	});

	// Could not find the buffer
	if (result == m_images.end())
	{
		throw CriticalVulkanError("Specified buffer cannot be freed, it does not exist.");
	}

	// Free the buffer
	vmaDestroyImage(m_allocator, image.image, image.allocation);

	// Remove the image from the container
	m_images.erase(result);
}

void* MemoryManager::MapBuffer(const VulkanBuffer& buffer)
{
	void* data = nullptr;
	vmaMapMemory(m_allocator, buffer.allocation, &data);

	return data;
}

void MemoryManager::UnMapBuffer(const VulkanBuffer& buffer)
{
	vmaUnmapMemory(m_allocator, buffer.allocation);
}

const VulkanBuffer& MemoryManager::Allocate(const BufferAllocationInfo& buffer_info) noexcept(false)
{
	VulkanBuffer buffer = {};

	auto result = vmaCreateBuffer(
		m_allocator,
		&buffer_info.buffer_create_info,
		&buffer_info.allocation_info,
		&buffer.buffer,
		&buffer.allocation,
		nullptr);

	buffer.id = CreateNewID();

	if (result != VK_SUCCESS)
	{
		throw CriticalVulkanError("Could not create a buffer.");
	}

	// Get the allocation information
	VmaAllocationInfo alloc_info = {};
	vmaGetAllocationInfo(m_allocator, buffer.allocation, &alloc_info);

	// Save the size and offset for easy access in the future
	buffer.offset = alloc_info.offset;
	buffer.size = alloc_info.size;

	// Save the buffer
	m_buffers.push_back(buffer);
	return m_buffers[m_buffers.size() - 1];
}

const VulkanImage& MemoryManager::Allocate(const ImageAllocationInfo& image_info) noexcept(false)
{
	VulkanImage image = {};

	auto result = vmaCreateImage(
		m_allocator,
		&image_info.image_create_info,
		&image_info.allocation_info,
		&image.image,
		&image.allocation,
		nullptr);

	image.id = CreateNewID();

	if (result != VK_SUCCESS)
	{
		throw CriticalVulkanError("Could not create an image.");
	}

	// Save the image
	m_images.push_back(image);
	return m_images[m_images.size() - 1];
}

const VmaAllocator& MemoryManager::GetVMAAllocation() const noexcept(true)
{
	return m_allocator;
}

MemoryManager::MemoryManager()
	: m_is_initialized(false)
{}

std::uint64_t MemoryManager::CreateNewID() noexcept(true)
{
	static std::uint64_t COUNTER = 0;
	return COUNTER++;
}
