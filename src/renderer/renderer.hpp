#pragma once

//////////////////////////////////////////////////////////////////////////

// Spdlog (including it here to avoid the "APIENTRY": macro redefinition warning)
#include <spdlog/spdlog.h>

// GLFW
#include <glfw/glfw3.h>

// Vulkan
#include <vulkan/vulkan.h>

//////////////////////////////////////////////////////////////////////////

// C++ standard
#include <vector>

//////////////////////////////////////////////////////////////////////////

namespace vkc
{
	struct QueueFamilyIndices
	{
		uint32_t graphics;
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

		static VKAPI_ATTR VkBool32 VKAPI_CALL DebugMessageCallback(
			VkDebugUtilsMessageSeverityFlagBitsEXT severity,
			VkDebugUtilsMessageTypeFlagsEXT type,
			const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
			void* user_data);

	private:
		GLFWwindow* m_window;

		VkInstance m_instance;
		VkDebugUtilsMessengerEXT m_debug_messenger;
	};
}