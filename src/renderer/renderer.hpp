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
#include <string>
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
		void SetupWindow(uint32_t width, uint32_t height, const char* title);

		GLFWwindow* const GetHandle() const;

	private:
		void CreateInstance();

	private:
		GLFWwindow* m_window;

		VkInstance m_instance;
	};
}