// Application
#include "miscellaneous/exceptions.hpp"
#include "vulkan_device.hpp"
#include "vulkan_instance.hpp"
#include "vulkan_swapchain.hpp"

// Application (order of include matters, hence it's this far down)
#include "core/window.hpp"

// GLFW
#include <glfw/glfw3.h>

// C++ standard
#include <algorithm>
#include <limits>

using namespace vkc::vk_wrapper;
using namespace vkc;

void VulkanSwapchain::CreateSurface(
	const VulkanInstance& instance,
	const Window& window) noexcept(false)
{
	if (glfwCreateWindowSurface(
		instance.GetNative(),
		window.GetNative(),
		nullptr,
		&m_surface) != VK_SUCCESS)
	{
		throw exception::CriticalVulkanError("Could not create a surface.");
	}
}

void VulkanSwapchain::Create(
	const VulkanDevice& device,
	const Window& window) noexcept(false)
{
	// Query the swapchain support details
	m_support_details = QuerySwapchainSupport(device);

	// Create the swapchain itself
	CreateSwapchain(device, window);

	// Get hold of the swapchain images
	GetSwapchainImages(device);

	// Create an image view for each image
	CreateSwapchainImagesImageViews(device);
}

void VulkanSwapchain::DestroySurface(const VulkanInstance& instance) const noexcept(true)
{
	vkDestroySurfaceKHR(instance.GetNative(), m_surface, nullptr);
}

const VkSurfaceKHR& VulkanSwapchain::GetSurfaceNative() const noexcept(true)
{
	return m_surface;
}

void VulkanSwapchain::Destroy(const VulkanDevice& device) const noexcept(true)
{
	DestroySwapchainResources(device);
}

const VkSwapchainKHR& VulkanSwapchain::GetNative() const noexcept(true)
{
	return m_swapchain;
}

const VkFormat& VulkanSwapchain::GetFormat() const noexcept(true)
{
	return m_swapchain_format;
}

const VkExtent2D& VulkanSwapchain::GetExtent() const noexcept(true)
{
	return m_swapchain_extent;
}

const std::vector<VkImage>& VulkanSwapchain::GetImages() const noexcept(true)
{
	return m_swapchain_images;
}

const std::vector<VkImageView>& VulkanSwapchain::GetImageViews() const noexcept(true)
{
	return m_swapchain_image_views;
}

SwapchainSupportDetails VulkanSwapchain::QuerySwapchainSupport(
	const VulkanDevice& device) const noexcept(false)
{
	std::uint32_t format_count = 0;
	std::uint32_t present_mode_count = 0;
	SwapchainSupportDetails support_details = {};

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
		device.GetPhysicalDeviceNative(),
		m_surface,
		&support_details.capabilities);

	// Query the surface formats
	vkGetPhysicalDeviceSurfaceFormatsKHR(
		device.GetPhysicalDeviceNative(),
		m_surface,
		&format_count,
		nullptr);

	if (format_count == 0)
	{
		// No valid format available
		throw exception::CriticalVulkanError("No valid surface format found.");
	}

	// Store the surface formats
	support_details.formats.resize(format_count);
	vkGetPhysicalDeviceSurfaceFormatsKHR(
		device.GetPhysicalDeviceNative(),
		m_surface,
		&format_count,
		support_details.formats.data());

	// Query the surface present modes
	vkGetPhysicalDeviceSurfacePresentModesKHR(
		device.GetPhysicalDeviceNative(),
		m_surface,
		&present_mode_count,
		nullptr);

	if (present_mode_count == 0)
	{
		// No valid present mode available
		throw exception::CriticalVulkanError("No valid surface present mode found.");
	}

	// Store the surface present modes
	support_details.present_modes.resize(present_mode_count);
	vkGetPhysicalDeviceSurfacePresentModesKHR(
		device.GetPhysicalDeviceNative(),
		m_surface,
		&present_mode_count,
		support_details.present_modes.data());

	if (support_details.formats.empty() ||
		support_details.present_modes.empty())
	{
		throw exception::CriticalVulkanError("Swapchain support is not adequate.");
	}

	return support_details;
}

void VulkanSwapchain::CreateSwapchain(
	const VulkanDevice& device,
	const Window& window) noexcept(false)
{
	// Find the best surface format to use
	auto surface_format = FindBestSurfaceFormat();
	auto surface_extent = FindSurfaceExtent(window);
	auto surface_present_mode = FindBestSurfacePresentMode();

	// Make sure to use one more image than the recommended amount
	auto image_count = m_support_details.capabilities.minImageCount + 1;

	// Do not exceed the maximum number of allowed images
	if (m_support_details.capabilities.maxImageCount > 0 &&
		image_count > m_support_details.capabilities.maxImageCount)
	{
		image_count = m_support_details.capabilities.maxImageCount;
	}

	auto queue_family_indices = device.GetQueueFamilyIndices();
	std::uint32_t queue_families[] = {
		queue_family_indices.graphics_family_index.value().first,
		queue_family_indices.present_family_index.value().first,
		queue_family_indices.compute_family_index.value().first
	};

	VkSwapchainCreateInfoKHR create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	create_info.surface = m_surface;
	create_info.minImageCount = image_count;
	create_info.imageFormat = surface_format.format;
	create_info.imageColorSpace = surface_format.colorSpace;
	create_info.imageExtent = surface_extent;
	create_info.imageArrayLayers = 1;
	create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	create_info.preTransform = m_support_details.capabilities.currentTransform;
	create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	create_info.presentMode = surface_present_mode;
	create_info.clipped = VK_TRUE;
	create_info.oldSwapchain = VK_NULL_HANDLE;
	create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;

	auto result = vkCreateSwapchainKHR(
		device.GetLogicalDeviceNative(),
		&create_info,
		nullptr,
		&m_swapchain);

	if (result != VK_SUCCESS)
	{
		throw exception::CriticalVulkanError("Could not create a swapchain.");
	}

	// Save format and extent for future use
	m_swapchain_format = surface_format.format;
	m_swapchain_extent = surface_extent;
}

VkSurfaceFormatKHR VulkanSwapchain::FindBestSurfaceFormat() const noexcept(true)
{
	if (m_support_details.formats.size() == 1 &&
		m_support_details.formats[0].format == VK_FORMAT_UNDEFINED)
	{
		// Surface has no preferred format, use our own preferred format
		return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
	}

	// Find the most optimal format out of all preferred formats
	for (const auto& format : m_support_details.formats)
	{
		// See if our own preferred format is supported
		if (format.format == VK_FORMAT_B8G8R8A8_UNORM &&
			format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			return format;
		}
	}

	// Just use the default format
	return m_support_details.formats[0];
}

VkExtent2D VulkanSwapchain::FindSurfaceExtent(const Window& window) const noexcept(true)
{
	// Use the default extent
	if (m_support_details.capabilities.currentExtent.width != (std::numeric_limits<std::uint32_t>::max)())
	{
		return m_support_details.capabilities.currentExtent;
	}

	// Use the resolution that matches the window within the given extent the best
	int width, height;
	glfwGetFramebufferSize(window.GetNative(), &width, &height);

	VkExtent2D extent = {};
	extent.width = static_cast<std::uint32_t>(width);
	extent.height = static_cast<std::uint32_t>(height);

	auto min_extent_width = m_support_details.capabilities.minImageExtent.width;
	auto max_extent_width = m_support_details.capabilities.maxImageExtent.width;
	auto min_extent_height = m_support_details.capabilities.minImageExtent.height;
	auto max_extent_height = m_support_details.capabilities.maxImageExtent.height;

	extent.width = (std::max)(min_extent_width, (std::min)(max_extent_width, extent.width));
	extent.height = (std::max)(min_extent_height, (std::min)(max_extent_height, extent.height));

	return extent;
}

VkPresentModeKHR VulkanSwapchain::FindBestSurfacePresentMode() const noexcept(true)
{
	for (const auto& mode : m_support_details.present_modes)
	{
		if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			// Mailbox is preferred
			return mode;
		}
		else if (mode == VK_PRESENT_MODE_IMMEDIATE_KHR)
		{
			// Less ideal, but still fine
			return mode;
		}
	}

	// As a last resort, fall back to the mode that is guaranteed to be available
	return VK_PRESENT_MODE_FIFO_KHR;
}

void VulkanSwapchain::GetSwapchainImages(
	const VulkanDevice& device) noexcept(true)
{
	std::uint32_t image_count = 0;
	vkGetSwapchainImagesKHR(
		device.GetLogicalDeviceNative(),
		m_swapchain,
		&image_count,
		nullptr);
	
	m_swapchain_images.resize(image_count);
	vkGetSwapchainImagesKHR(
		device.GetLogicalDeviceNative(),
		m_swapchain,
		&image_count,
		m_swapchain_images.data());
}

void VulkanSwapchain::CreateSwapchainImagesImageViews(
	const VulkanDevice& device) noexcept(false)
{
	m_swapchain_image_views.resize(m_swapchain_images.size());

	std::uint32_t index = 0;
	for (const auto& image : m_swapchain_images)
	{
		VkImageViewCreateInfo create_info = {};
		create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		create_info.image = image;
		create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		create_info.format = m_swapchain_format;
		create_info.components = {
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY
		};
		create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		create_info.subresourceRange.baseMipLevel = 0;
		create_info.subresourceRange.levelCount = 1;
		create_info.subresourceRange.baseArrayLayer = 0;
		create_info.subresourceRange.layerCount = 1;

		auto result = vkCreateImageView(
			device.GetLogicalDeviceNative(),
			&create_info,
			nullptr,
			&m_swapchain_image_views[index]);

		if (result != VK_SUCCESS)
		{
			throw exception::CriticalVulkanError("Could not create image view.");
		}

		++index;
	}
}

void VulkanSwapchain::DestroySwapchainResources(const VulkanDevice& device) const noexcept(true)
{
	for (const auto& image_view : m_swapchain_image_views)
	{
		vkDestroyImageView(device.GetLogicalDeviceNative(), image_view, nullptr);
	}

	vkDestroySwapchainKHR(device.GetLogicalDeviceNative(), m_swapchain, nullptr);
}
