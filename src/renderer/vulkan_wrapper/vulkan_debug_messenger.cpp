// Application
#include "miscellaneous/exceptions.hpp"
#include "vulkan_debug_messenger.hpp"
#include "vulkan_instance.hpp"

// Spdlog
#include <spdlog/spdlog.h>

using namespace vkc::vk_wrapper;

void VulkanDebugMessenger::Create(VulkanInstance instance) noexcept(false)
{
	VkDebugUtilsMessengerCreateInfoEXT create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	create_info.messageSeverity = ConfigureMessageSeverity();
	create_info.messageType = ConfigureMessageType();
	create_info.pfnUserCallback = DebugMessageCallback;

	auto result = CreateDebugUtilsMessengerEXT(
		instance.GetNative(),
		&create_info,
		nullptr,
		&m_debug_messenger);

	if (result != VK_SUCCESS)
	{
		throw exception::CriticalVulkanError("Could not create a debug messenger.");
	}
}

void VulkanDebugMessenger::Destroy(VulkanInstance instance) const noexcept(true)
{
	DestroyDebugUtilsMessengerEXT(instance.GetNative(), nullptr, m_debug_messenger);
}

VkResult VulkanDebugMessenger::CreateDebugUtilsMessengerEXT(
	VkInstance instance,
	const VkDebugUtilsMessengerCreateInfoEXT* create_info,
	const VkAllocationCallbacks* allocator,
	VkDebugUtilsMessengerEXT* debug_messenger) const noexcept(true)
{
	auto function = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
		vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));

	if (function)
	{
		return function(instance, create_info, allocator, debug_messenger);
	}

	return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void VulkanDebugMessenger::DestroyDebugUtilsMessengerEXT(
	VkInstance instance,
	const VkAllocationCallbacks* allocator,
	const VkDebugUtilsMessengerEXT& debug_messenger) const noexcept(true)
{
	auto function = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
		vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));

	if (function)
	{
		function(instance, debug_messenger, allocator);
	}
}

VkDebugUtilsMessageSeverityFlagsEXT VulkanDebugMessenger::ConfigureMessageSeverity() const noexcept(true)
{
	// Show verbose validation layer warnings and errors
	return	VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT	|
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT	|
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
}

VkDebugUtilsMessageTypeFlagsEXT VulkanDebugMessenger::ConfigureMessageType() const noexcept(true)
{
	// Only log general, validation layer, and performance warnings
	return	VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT		|
			VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT	|
			VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
}

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugMessenger::DebugMessageCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT severity,
	VkDebugUtilsMessageTypeFlagsEXT type,
	const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
	void* user_data) noexcept(true)
{
	// Suppress "unreferenced formal parameter" warning when using warning level 4
	type, user_data;

	if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
	{
		// Log a validation layer error
		spdlog::error(callback_data->pMessage);
	}
	else if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
	{
		// Log a validation layer warning
		spdlog::warn(callback_data->pMessage);
	}

	return VK_FALSE;
}
