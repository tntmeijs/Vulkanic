// Application
#include "miscellaneous/exceptions.hpp"
#include "vulkan_command_buffer.hpp"
#include "vulkan_command_pool.hpp"
#include "vulkan_device.hpp"

using namespace vkc::exception;
using namespace vkc::vk_wrapper;

VulkanCommandBuffer::VulkanCommandBuffer() noexcept(true)
{}

VulkanCommandBuffer::~VulkanCommandBuffer() noexcept(true)
{}

void VulkanCommandBuffer::Create(
	const VulkanDevice& device,
	const VulkanCommandPool& command_pool,
	std::uint32_t command_buffer_count,
	bool is_primary) noexcept(false)
{
	VkCommandBufferAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	alloc_info.commandBufferCount = command_buffer_count;
	alloc_info.commandPool = command_pool.GetNative();
	alloc_info.level = is_primary ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY;

	m_command_buffers.resize(command_buffer_count);

	auto result = vkAllocateCommandBuffers(device.GetLogicalDeviceNative(), &alloc_info, m_command_buffers.data());

	if (result != VK_SUCCESS)
	{
		throw CriticalVulkanError("Could not allocate command buffers.");
	}
}

void VulkanCommandBuffer::Destroy(const VulkanDevice& device, const VulkanCommandPool& command_pool) const noexcept(true)
{
	vkFreeCommandBuffers(
		device.GetLogicalDeviceNative(),
		command_pool.GetNative(),
		static_cast<std::uint32_t>(m_command_buffers.size()),
		m_command_buffers.data());
}

const VkCommandBuffer& VulkanCommandBuffer::GetNative() const noexcept(false)
{
	// Throws an out of range exception as long as the class is uninitialized
	return m_command_buffers[0];
}

const VkCommandBuffer& VulkanCommandBuffer::GetNative(std::uint32_t index) const noexcept(false)
{
	// Throws and out of range exception if the specified index is invalid
	return m_command_buffers[index];
}

void VulkanCommandBuffer::BeginRecording(CommandBufferUsage usage) const noexcept(false)
{
	VkCommandBufferBeginInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	info.flags = static_cast<VkCommandBufferUsageFlags>(usage);

	// Throws an out of range exception as long as the class is uninitialized
	auto result = vkBeginCommandBuffer(m_command_buffers[0], &info);

	if (result != VK_SUCCESS)
	{
		throw CriticalVulkanError("Could not begin regarding to a command buffer.");
	}
}

void VulkanCommandBuffer::BeginRecording(std::uint32_t index, CommandBufferUsage usage) const noexcept(false)
{
	VkCommandBufferBeginInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	info.flags = static_cast<VkCommandBufferUsageFlags>(usage);

	// Throws and out of range exception if the specified index is invalid
	auto result = vkBeginCommandBuffer(m_command_buffers[index], &info);

	if (result != VK_SUCCESS)
	{
		throw CriticalVulkanError("Could not begin regarding to a command buffer.");
	}
}

void VulkanCommandBuffer::StopRecording() const noexcept(false)
{
	// Throws an out of range exception as long as the class is uninitialized
	auto result = vkEndCommandBuffer(m_command_buffers[0]);

	if (result != VK_SUCCESS)
	{
		throw CriticalVulkanError("Recording to command buffer failed.");
	}
}

void VulkanCommandBuffer::StopRecording(std::uint32_t index) const noexcept(false)
{
	// Throws and out of range exception if the specified index is invalid
	auto result = vkEndCommandBuffer(m_command_buffers[index]);

	if (result != VK_SUCCESS)
	{
		throw CriticalVulkanError("Recording to command buffer failed.");
	}
}
