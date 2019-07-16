// Application
#include "miscellaneous/exceptions.hpp"
#include "vulkan_device.hpp"
#include "vulkan_render_pass.hpp"

// C++ standard
#include <vector>

using namespace vkc::exception;
using namespace vkc::vk_wrapper;

void VulkanRenderPass::Create(
	const VulkanDevice& device,
	const VulkanRenderPassInfo& info) noexcept(false)
{
	VkRenderPassCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	create_info.attachmentCount = static_cast<std::uint32_t>(
		info.attachment_descriptions.size());
	create_info.subpassCount = static_cast<std::uint32_t>(
		info.attachment_descriptions.size());
	create_info.dependencyCount = static_cast<std::uint32_t>(
		info.subpass_dependencies.size());
	create_info.pAttachments = info.attachment_descriptions.data();
	create_info.pSubpasses = info.subpass_descriptions.data();
	create_info.pDependencies = info.subpass_dependencies.data();

	auto result = vkCreateRenderPass(
		device.GetLogicalDeviceNative(),
		&create_info,
		nullptr,
		&m_render_pass);

	if (result != VK_SUCCESS)
	{
		throw CriticalVulkanError("Could not create render pass.");
	}
}

void VulkanRenderPass::Destroy(const VulkanDevice& device) const noexcept(true)
{
	vkDestroyRenderPass(device.GetLogicalDeviceNative(), m_render_pass, nullptr);
}

const VkRenderPass& VulkanRenderPass::GetNative() const noexcept(true)
{
	return m_render_pass;
}
