// Application
#include "miscellaneous/exceptions.hpp"
#include "renderer/memory_manager/memory_manager.hpp"
#include "vulkan_device.hpp"
#include "vulkan_texture.hpp"
#include "vulkan_utility.hpp"

// STB image
#include <stb_image.h>

// C++ standard
#include <string>

using namespace vkc::exception;
using namespace vkc::memory;
using namespace vkc::vk_wrapper::utility;
using namespace vkc::vk_wrapper;

VulkanTexture::VulkanTexture()
	: m_width(0)
	, m_height(0)
	, m_channel_count(0)
	, m_format(VK_FORMAT_UNDEFINED)
	, m_image_view(VK_NULL_HANDLE)
	, m_image(nullptr)
{}

VulkanTexture::~VulkanTexture()
{}

void VulkanTexture::Create(
	const std::string_view path,
	VkFormat format,
	const VulkanDevice& device,
	const VulkanCommandPool& command_pool) noexcept(false)
{
	m_format = format;

	// Load image pixel data from file
	unsigned char* pixel_data = LoadDataFromFile(path);

	// Number of bytes per image color channel
	std::uint32_t bytes_per_channel = VulkanFormatToBytesPerChannel(format);

	// Size of the texture data
	VkDeviceSize data_size = static_cast<VkDeviceSize>(m_width * m_height * m_channel_count * bytes_per_channel);

	// Staging buffer used to upload the texture data to device memory
	auto texture_staging_buffer = CreateStagingBuffer(data_size);

	// Copy texture data to the staging buffer
	auto data = MemoryManager::GetInstance().MapBuffer(texture_staging_buffer);
	memcpy(data, pixel_data, data_size);
	MemoryManager::GetInstance().UnMapBuffer(texture_staging_buffer);

	// Clean-up the image pixel data
	stbi_image_free(pixel_data);

	// Create the Vulkan image object
	CreateImage();

	// Transition image layout so it can be used as a copy destination
	TransitionImageLayout(device, command_pool, m_image->image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	// Now that the image can be copied to, copy the staging buffer to the device local memory for the image
	CopyStagingBufferToDeviceLocal(texture_staging_buffer, device, command_pool);

	// Transition image layout so it can be used in the fragment shader to sample from
	TransitionImageLayout(device, command_pool, m_image->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	// Create an image view for the newly created image
	CreateImageView(device);

	// Delete the staging buffer
	MemoryManager::GetInstance().Free(texture_staging_buffer);
}

void VulkanTexture::Destroy(const VulkanDevice& device)
{
	vkDestroyImageView(device.GetLogicalDeviceNative(), m_image_view, nullptr);
	MemoryManager::GetInstance().Free(*m_image);
}

const VulkanImage& VulkanTexture::GetImage() const noexcept(true)
{
	return *m_image;
}

const VkImageView& VulkanTexture::GetImageView() const noexcept(true)
{
	return m_image_view;
}

unsigned char* vkc::vk_wrapper::VulkanTexture::LoadDataFromFile(const std::string_view path) noexcept(false)
{
	// Attempt to load the image from the specified file
	unsigned char* data = stbi_load(path.data(), &m_width, &m_height, &m_channel_count, 0);

	if (!data)
	{
		// Failed to load the image
		throw CriticalIOError("Unable to load the texture data at: " + std::string(path));
	}

	return data;
}

const VulkanBuffer& VulkanTexture::CreateStagingBuffer(const VkDeviceSize buffer_size) noexcept(true)
{
	memory::BufferAllocationInfo texture_staging_buffer_info = {};
	texture_staging_buffer_info.buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	texture_staging_buffer_info.buffer_create_info.size = buffer_size;
	texture_staging_buffer_info.buffer_create_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	texture_staging_buffer_info.buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	texture_staging_buffer_info.allocation_info.usage = VMA_MEMORY_USAGE_CPU_ONLY;

	return MemoryManager::GetInstance().Allocate(texture_staging_buffer_info);
}

void VulkanTexture::CreateImage() noexcept(true)
{
	memory::ImageAllocationInfo texture_image_allocation_info = {};
	texture_image_allocation_info.image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	texture_image_allocation_info.image_create_info.imageType = VK_IMAGE_TYPE_2D;
	texture_image_allocation_info.image_create_info.extent.width = m_width;
	texture_image_allocation_info.image_create_info.extent.height = m_height;
	texture_image_allocation_info.image_create_info.extent.depth = 1;
	texture_image_allocation_info.image_create_info.mipLevels = 1;
	texture_image_allocation_info.image_create_info.arrayLayers = 1;
	texture_image_allocation_info.image_create_info.format = m_format;
	texture_image_allocation_info.image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
	texture_image_allocation_info.image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	texture_image_allocation_info.image_create_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	texture_image_allocation_info.image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	texture_image_allocation_info.image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;

	texture_image_allocation_info.allocation_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;

	m_image = std::make_unique<VulkanImage>(MemoryManager::GetInstance().Allocate(texture_image_allocation_info));
}

void vkc::vk_wrapper::VulkanTexture::CopyStagingBufferToDeviceLocal(
	const memory::VulkanBuffer& staging_buffer,
	const VulkanDevice& device,
	const VulkanCommandPool& command_pool)
{
	VulkanCommandBuffer cmd_buffer = {};
	cmd_buffer.Create(device, command_pool, 1);
	cmd_buffer.BeginRecording(CommandBufferUsage::OneTimeSubmit);

	VkBufferImageCopy copy_region = {};

	// Padding
	copy_region.bufferOffset = 0;
	copy_region.bufferRowLength = 0;
	copy_region.bufferImageHeight = 0;

	// Mip and array levels
	copy_region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	copy_region.imageSubresource.baseArrayLayer = 0;
	copy_region.imageSubresource.mipLevel = 0;
	copy_region.imageSubresource.layerCount = 1;

	// Copy the entire image
	copy_region.imageOffset = { 0, 0, 0 };
	copy_region.imageExtent = { static_cast<std::uint32_t>(m_width), static_cast<std::uint32_t>(m_height), 1 };

	// Queue the copy command
	vkCmdCopyBufferToImage(cmd_buffer.GetNative(), staging_buffer.buffer, m_image->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy_region);

	auto graphics_queue = device.GetQueueNativeOfType(VulkanQueueType::Graphics);

	// Execute the staging buffer to device local memory copy
	cmd_buffer.StopRecording();
	cmd_buffer.Submit(graphics_queue);
	vkQueueWaitIdle(graphics_queue);

	// Command buffer is no longer needed
	cmd_buffer.Destroy(device, command_pool);
}

void VulkanTexture::CreateImageView(const VulkanDevice& device) noexcept(false)
{
	VkImageViewCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	create_info.image = m_image->image;
	create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
	create_info.format = m_format;
	create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	create_info.subresourceRange.layerCount = 1;
	create_info.subresourceRange.levelCount = 1;
	create_info.subresourceRange.baseArrayLayer = 0;
	create_info.subresourceRange.baseMipLevel = 0;
	create_info.components = {
		VK_COMPONENT_SWIZZLE_IDENTITY,
		VK_COMPONENT_SWIZZLE_IDENTITY,
		VK_COMPONENT_SWIZZLE_IDENTITY,
		VK_COMPONENT_SWIZZLE_IDENTITY
	};

	if (vkCreateImageView(device.GetLogicalDeviceNative(), &create_info, nullptr, &m_image_view) != VK_SUCCESS)
	{
		throw CriticalVulkanError("Unable to create an image view.");
	}
}
