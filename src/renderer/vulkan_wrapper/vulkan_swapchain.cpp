// Application
#include "miscellaneous/exceptions.hpp"
#include "vulkan_device.hpp"
#include "vulkan_instance.hpp"
#include "vulkan_swapchain.hpp"

// Application (order of include matters, hence it's this far down)
#include "core/window.hpp"

// GLFW
#include <glfw/glfw3.h>

using namespace vkc;
using namespace vkc::vk_wrapper;

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

void VulkanSwapchain::Create(const VulkanDevice& device) noexcept(false)
{
	// Query the swapchain support details
	m_support_details = QuerySwapchainSupport(device);

	// Create the swapchain itself
	m_swapchain = CreateSwapchain(device);
}

void VulkanSwapchain::DestroySurface(const VulkanInstance& instance) const noexcept(true)
{
	vkDestroySurfaceKHR(instance.GetNative(), m_surface, nullptr);
}

const VkSurfaceKHR& VulkanSwapchain::GetSurfaceNative() const noexcept(true)
{
	return m_surface;
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

VkSwapchainKHR VulkanSwapchain::CreateSwapchain(
	const VulkanDevice& device,
	const SwapchainSupportDetails& support_details) const noexcept(false)
{
	VkSwapchainKHR swapchain;

	// Find the best surface format to use
	auto format = FindBestSurfaceFormat(support_details);






	return swapchain;
}

VkSurfaceFormatKHR VulkanSwapchain::FindBestSurfaceFormat(
	const SwapchainSupportDetails& support_details) const noexcept(true)
{
	CONTINUE HERE
	return VkSurfaceFormatKHR();
}
