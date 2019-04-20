#pragma once

//////////////////////////////////////////////////////////////////////////

// Spdlog (including it here to avoid the "APIENTRY": macro redefinition warning)
#include <spdlog/spdlog.h>

// GLFW
#include <glfw/glfw3.h>

// Vulkan
#include <vulkan/vulkan.h>

//////////////////////////////////////////////////////////////////////////

namespace vkc
{
	class Renderer
	{
	public:
		Renderer();
		~Renderer();

		void InitializeVulkan();
		void SetupWindow(uint32_t width, uint32_t height, const char* title);
		void Prepare();
		void Run();

		GLFWwindow* const GetHandle() const;

	private:
		GLFWwindow* m_window;
	};
}