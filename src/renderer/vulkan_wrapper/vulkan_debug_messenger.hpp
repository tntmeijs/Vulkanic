#ifndef VULKAN_DEBUG_MESSENGER_HPP
#define VULKAN_DEBUG_MESSENGER_HPP

// Vulkan
#include <vulkan/vulkan.h>

namespace vkc::vk_wrapper
{
	class VulkanInstance;
	
	class VulkanDebugMessenger
	{
	public:
		VulkanDebugMessenger() noexcept(true) : m_debug_messenger(VK_NULL_HANDLE) {}
		~VulkanDebugMessenger() noexcept(true) {}

		/** Create a Vulkan debug messenger */
		void Create(VulkanInstance instance) noexcept(false);

		/** Destroy an existing Vulkan debug messenger */
		void Destroy(VulkanInstance instance) const noexcept(true);

	private:
		/** Same as the API function, but wraps the "vkGetInstanceProcAddr" call */
		VkResult CreateDebugUtilsMessengerEXT(
			VkInstance instance,
			const VkDebugUtilsMessengerCreateInfoEXT* create_info,
			const VkAllocationCallbacks* allocator,
			VkDebugUtilsMessengerEXT* debug_messenger) const noexcept(true);

		/** Same as the API function, but wraps the "vkGetInstanceProcAddr" call */
		void DestroyDebugUtilsMessengerEXT(
			VkInstance instance,
			const VkAllocationCallbacks* allocator,
			const VkDebugUtilsMessengerEXT& debug_messenger) const noexcept(true);

		/** Configure the debug utilities message severity flags */
		VkDebugUtilsMessageSeverityFlagsEXT ConfigureMessageSeverity() const noexcept(true);

		/** Configure the debug utilities message type flags */
		VkDebugUtilsMessageTypeFlagsEXT ConfigureMessageType() const noexcept(true);

		/** Callback function used to log Vulkan validation layer messages */
		static VKAPI_ATTR VkBool32 VKAPI_CALL DebugMessageCallback(
			VkDebugUtilsMessageSeverityFlagBitsEXT severity,
			VkDebugUtilsMessageTypeFlagsEXT type,
			const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
			void* user_data) noexcept(true);

	private:
		VkDebugUtilsMessengerEXT m_debug_messenger;
	};
}

#endif
