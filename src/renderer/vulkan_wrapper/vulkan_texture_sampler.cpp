// Application
#include "miscellaneous/exceptions.hpp"
#include "vulkan_device.hpp"
#include "vulkan_texture_sampler.hpp"

using namespace vkc::exception;
using namespace vkc::vk_wrapper;

VulkanTextureSampler::VulkanTextureSampler() noexcept(true)
	: m_sampler(VK_NULL_HANDLE)
{}

VulkanTextureSampler::~VulkanTextureSampler() noexcept(true)
{}

void VulkanTextureSampler::Create(const VulkanDevice& device) noexcept(true)
{
	// Call the overloaded create function using the default settings of the sampler settings structure
	Create( device, {} );
}

void VulkanTextureSampler::Create(const VulkanDevice& device, const TextureSamplerSettings& sampler_settings) noexcept(false)
{
	// Use the specified settings to create a sampler
	VkSamplerCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	info.minFilter = static_cast<VkFilter>(sampler_settings.min_filter);
	info.magFilter = static_cast<VkFilter>(sampler_settings.mag_filter);
	info.addressModeU = static_cast<VkSamplerAddressMode>(sampler_settings.behavior_u);
	info.addressModeV = static_cast<VkSamplerAddressMode>(sampler_settings.behavior_v);
	info.addressModeW = static_cast<VkSamplerAddressMode>(sampler_settings.behavior_w);
	info.anisotropyEnable = sampler_settings.anisotropy_enabled;
	info.maxAnisotropy = sampler_settings.anisotropy_value;
	info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	info.unnormalizedCoordinates = !sampler_settings.use_normalized_coordinates;
	info.compareEnable = sampler_settings.comparison_enabled;
	info.compareOp = static_cast<VkCompareOp>(sampler_settings.compare_operation);
	info.mipmapMode = static_cast<VkSamplerMipmapMode>(sampler_settings.mipmap_mode);
	info.mipLodBias = sampler_settings.mipmap_lod_bias;
	info.minLod = sampler_settings.min_lod;
	info.maxLod = sampler_settings.max_lod;

	// Create the actual sampler object
	auto result = vkCreateSampler(device.GetLogicalDeviceNative(), &info, nullptr, &m_sampler);

	if (result != VK_SUCCESS)
	{
		throw CriticalVulkanError("Could not create a texture sampler.");
	}
}

void VulkanTextureSampler::Destroy(const VulkanDevice& device) const noexcept(true)
{
	vkDestroySampler(device.GetLogicalDeviceNative(), m_sampler, nullptr);
}

const VkSampler& VulkanTextureSampler::GetNative() const noexcept(true)
{
	return m_sampler;
}
