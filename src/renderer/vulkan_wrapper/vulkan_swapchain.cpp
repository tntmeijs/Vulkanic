// Application
#include "miscellaneous/exceptions.hpp"
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

void VulkanSwapchain::DestroySurface(const VulkanInstance& instance) const noexcept(true)
{
	vkDestroySurfaceKHR(instance.GetNative(), m_surface, nullptr);
}

const VkSurfaceKHR& VulkanSwapchain::GetSurfaceNative() const noexcept(true)
{
	return m_surface;
}
