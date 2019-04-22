#pragma once

//////////////////////////////////////////////////////////////////////////

// Spdlog (including it here to avoid the "APIENTRY": macro redefinition warning)
#include <spdlog/spdlog.h>

// Vulkan
#include <vulkan/vulkan.h>

// GLFW
#include <glfw/glfw3.h>

//////////////////////////////////////////////////////////////////////////

// C++ standard
#include <optional>
#include <vector>

//////////////////////////////////////////////////////////////////////////

namespace vkc
{
	struct QueueFamilyIndices
	{
		std::optional<uint32_t> graphics_family_index;

		bool AllIndicesFound()
		{
			return graphics_family_index.has_value();
		}
	};

	class Renderer
	{
	public:
		Renderer();
		~Renderer();

		void InitializeVulkan();
		void SetupWindow();

		GLFWwindow* const GetHandle() const;

	private:
		void CreateInstance();
		bool CheckValidationLayerSupport();
		std::vector<const char*> GetRequiredExtensions();
		void SetUpDebugMessenger();
		void SelectPhysicalDevice();
		uint32_t RatePhysicalDeviceSuitability(const VkPhysicalDevice& physical_device);
		QueueFamilyIndices FindQueueFamiliesOfSelectedPhysicalDevice();
		void CreateLogicalDevice();
		void CreateSurface();

		static VKAPI_ATTR VkBool32 VKAPI_CALL DebugMessageCallback(
			VkDebugUtilsMessageSeverityFlagBitsEXT severity,
			VkDebugUtilsMessageTypeFlagsEXT type,
			const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
			void* user_data);

	private:
		GLFWwindow* m_window;
		QueueFamilyIndices m_queue_family_indices;

		VkInstance m_instance;
		VkDebugUtilsMessengerEXT m_debug_messenger;
		VkPhysicalDevice m_physical_device;
		VkDevice m_device;
		VkQueue m_graphics_queue;
		VkSurfaceKHR m_surface;
	};
}