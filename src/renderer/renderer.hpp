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
		std::optional<std::pair<uint32_t, uint32_t>> graphics_family_index;
		std::optional<std::pair<uint32_t, uint32_t>> present_family_index;
		std::optional<std::pair<uint32_t, uint32_t>> transfer_family_index;

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
		void Draw();
		void TriggerFramebufferResized();

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
		void CreateCommandPools();
		void CreateCommandBuffers();
		void CreateSynchronizationObjects();
		void RecreateSwapchain();
		void CleanUpSwapchain();
		void CreateTransferCommandBuffer();
		void CreateVertexBuffer();
		void CreateDescriptorSetLayout();
		
		static uint32_t FindMemoryType(
			uint32_t type_filter,
			VkMemoryPropertyFlags properties,
			const VkPhysicalDevice& physical_device);

		static VKAPI_ATTR VkBool32 VKAPI_CALL DebugMessageCallback(
			VkDebugUtilsMessageSeverityFlagBitsEXT severity,
			VkDebugUtilsMessageTypeFlagsEXT type,
			const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
			void* user_data);

		static std::vector<char> ReadSPRIVFromFile(const char* file);

		static void CreateBuffer(
			VkDeviceSize size,
			VkBufferUsageFlags usage,
			VkMemoryPropertyFlags properties,
			VkBuffer& buffer,
			VkDeviceMemory& buffer_memory,
			const QueueFamilyIndices& queue_family_indices,
			const VkDevice& device,
			const VkPhysicalDevice physical_device);

		static void CopyStagingBufferToDeviceLocalBuffer(
			const VkBuffer& source,
			const VkBuffer& destination,
			VkDeviceSize size,
			const VkCommandBuffer& transfer_command_buffer,
			const VkQueue& transfer_queue);

	private:
		GLFWwindow* m_window;
		QueueFamilyIndices m_queue_family_indices;
		SwapchainSupportDetails m_swap_chain_support;
		uint64_t m_frame_index;

		bool m_framebuffer_resized;

		VkInstance m_instance;
		VkDebugUtilsMessengerEXT m_debug_messenger;
		VkSurfaceKHR m_surface;
		VkPhysicalDevice m_physical_device;
		VkDevice m_device;
		VkQueue m_graphics_queue;
		VkQueue m_present_queue;
		VkQueue m_transfer_queue;
		VkSwapchainKHR m_swapchain;
		VkFormat m_swapchain_format;
		VkExtent2D m_swapchain_extent;
		VkRenderPass m_render_pass;
		VkDescriptorSetLayout m_camera_data_descriptor_set_layout;
		VkPipelineLayout m_pipeline_layout;
		VkPipeline m_graphics_pipeline;
		VkCommandPool m_graphics_command_pool;
		VkCommandPool m_transfer_command_pool;
		VkBuffer m_vertex_buffer;
		VkDeviceMemory m_vertex_buffer_memory;
		VkCommandBuffer m_transfer_command_buffer;

		std::vector<VkImage> m_swapchain_images;
		std::vector<VkImageView> m_swapchain_image_views;
		std::vector<VkFramebuffer> m_swapchain_framebuffers;
		std::vector<VkCommandBuffer> m_command_buffers;
		std::vector<VkSemaphore> m_in_flight_frame_image_available_semaphores;
		std::vector<VkSemaphore> m_in_flight_render_finished_semaphores;
		std::vector<VkFence> m_in_flight_fences;
	};
}