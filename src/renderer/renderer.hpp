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
		std::optional<uint32_t> present_family_index;

		bool AllIndicesFound()
		{
			return (graphics_family_index.has_value() && present_family_index.has_value());
		}
	};

	struct SwapchainSupportDetails
	{
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> present_modes;
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
		std::vector<const char*> GetRequiredInstanceExtensions();
		void SetUpDebugMessenger();
		void CreateSurface();
		void SelectPhysicalDevice();
		uint32_t RatePhysicalDeviceSuitability(const VkPhysicalDevice& physical_device);
		QueueFamilyIndices FindQueueFamiliesOfSelectedPhysicalDevice();
		void CreateLogicalDevice();
		bool CheckPhysicalDeviceExtensionSupport();
		void QuerySwapchainSupport();
		VkSurfaceFormatKHR ChooseSwapchainSurfaceFormat();
		VkPresentModeKHR ChooseSwapchainSurfacePresentMode();
		VkExtent2D ChooseSwapchainExtent();
		void CreateSwapchain();
		void CreateSwapchainImageViews();
		void CreateGraphicsPipeline();
		VkShaderModule CreateShaderModule(const std::vector<char>& spirv);
		void CreateRenderPass();
		void CreateFramebuffers();

		static VKAPI_ATTR VkBool32 VKAPI_CALL DebugMessageCallback(
			VkDebugUtilsMessageSeverityFlagBitsEXT severity,
			VkDebugUtilsMessageTypeFlagsEXT type,
			const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
			void* user_data);

		static std::vector<char> ReadSPRIVFromFile(const char* file);

	private:
		GLFWwindow* m_window;
		QueueFamilyIndices m_queue_family_indices;
		SwapchainSupportDetails m_swap_chain_support;

		VkInstance m_instance;
		VkDebugUtilsMessengerEXT m_debug_messenger;
		VkSurfaceKHR m_surface;
		VkPhysicalDevice m_physical_device;
		VkDevice m_device;
		VkQueue m_graphics_queue;
		VkQueue m_present_queue;
		VkSwapchainKHR m_swapchain;
		VkFormat m_swapchain_format;
		VkExtent2D m_swapchain_extent;
		VkRenderPass m_render_pass;
		VkPipelineLayout m_pipeline_layout;
		VkPipeline m_graphics_pipeline;

		std::vector<VkImage> m_swapchain_images;
		std::vector<VkImageView> m_swapchain_image_views;
		std::vector<VkFramebuffer> m_swapchain_framebuffers;
	};
}