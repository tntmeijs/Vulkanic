// Application
#include "miscellaneous/exceptions.hpp"
#include "vulkan_command_pool.hpp"
#include "vulkan_device.hpp"

using namespace vkc::exception;
using namespace vkc::vk_wrapper;

VulkanCommandPool::VulkanCommandPool() noexcept(true)
{}

VulkanCommandPool::~VulkanCommandPool() noexcept(true)
{}

void VulkanCommandPool::Create(const VulkanDevice & device, CommandPoolType type) noexcept(false)
{
	VkCommandPoolCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;

	auto queue_family_indices = device.GetQueueFamilyIndices();

	switch (type)
	{
		case vkc::vk_wrapper::CommandPoolType::Graphics:
			info.queueFamilyIndex = queue_family_indices.graphics_family_index->first;
			break;
		
		case vkc::vk_wrapper::CommandPoolType::Compute:
			info.queueFamilyIndex = queue_family_indices.compute_family_index->first;
			break;

		default:
			throw CriticalVulkanError("Invalid command pool type specified.");
			break;
	}

	auto result = vkCreateCommandPool(device.GetLogicalDeviceNative(), &info, nullptr, &m_command_pool);

	if (result != VK_SUCCESS)
	{
		throw CriticalVulkanError("Could not create a command pool.");
	}
}

void VulkanCommandPool::Destroy(const VulkanDevice& device) const noexcept(true)
{
	vkDestroyCommandPool(device.GetLogicalDeviceNative(), m_command_pool, nullptr);
}

const VkCommandPool& VulkanCommandPool::GetNative() const noexcept(true)
{
	return m_command_pool;
}
