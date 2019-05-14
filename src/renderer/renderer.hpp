#pragma once

// Application Vulkan wrappers
#include "vulkan_wrapper/vulkan_debug_messenger.hpp"
#include "vulkan_wrapper/vulkan_device.hpp"
#include "vulkan_wrapper/vulkan_instance.hpp"
#include "vulkan_wrapper/vulkan_swapchain.hpp"

// Application core
#include "core/window.hpp"

//////////////////////////////////////////////////////////////////////////

// Spdlog (including it here to avoid the "APIENTRY": macro redefinition warning)
#include <spdlog/spdlog.h>

//////////////////////////////////////////////////////////////////////////

// C++ standard
#include <optional>
#include <vector>

//////////////////////////////////////////////////////////////////////////

namespace vkc
{
	class Renderer
	{
	public:
		Renderer();
		~Renderer();

		void Initialize(const Window& window);
		void Draw(const Window& window);
		void Update();
		void TriggerFramebufferResized();

	private:
		void CreateGraphicsPipeline();
		VkShaderModule CreateShaderModule(const std::vector<char>& spirv);
		void CreateRenderPass();
		void CreateFramebuffers();
		void CreateCommandPools();
		void CreateCommandBuffers();
		void CreateSynchronizationObjects();
		void RecreateSwapchain(const Window& window);
		void CleanUpSwapchain();
		void CreateVertexBuffer();
		void CreateUniformBuffers();
		void CreateDescriptorPool();
		void CreateDescriptorSetLayout();
		void CreateDescriptorSets();
		
		static uint32_t FindMemoryType(
			uint32_t type_filter,
			VkMemoryPropertyFlags properties,
			const VkPhysicalDevice& physical_device);

		static std::vector<char> ReadSPRIVFromFile(const char* file);

		static void CreateBuffer(
			VkDeviceSize size,
			VkBufferUsageFlags usage,
			VkMemoryPropertyFlags properties,
			VkBuffer& buffer,
			VkDeviceMemory& buffer_memory,
			const VkDevice& device,
			const VkPhysicalDevice physical_device);

		static void CopyStagingBufferToDeviceLocalBuffer(
			const VkDevice& device,
			const VkBuffer& source,
			const VkBuffer& destination,
			VkDeviceSize size,
			const VkQueue& transfer_queue,
			const VkCommandPool pool);

	private:
		GLFWwindow* m_window;
		uint64_t m_frame_index;
		uint32_t m_current_swapchain_image_index;

		bool m_framebuffer_resized;

		VkRenderPass m_render_pass;
		VkDescriptorSetLayout m_camera_data_descriptor_set_layout;
		VkPipelineLayout m_pipeline_layout;
		VkPipeline m_graphics_pipeline;
		VkCommandPool m_graphics_command_pool;
		VkBuffer m_vertex_buffer;
		VkDeviceMemory m_vertex_buffer_memory;
		VkDescriptorPool m_descriptor_pool;

		std::vector<VkFramebuffer> m_swapchain_framebuffers;
		std::vector<VkCommandBuffer> m_command_buffers;
		std::vector<VkSemaphore> m_in_flight_frame_image_available_semaphores;
		std::vector<VkSemaphore> m_in_flight_render_finished_semaphores;
		std::vector<VkFence> m_in_flight_fences;
		std::vector<VkBuffer> m_camera_ubos;
		std::vector<VkDeviceMemory> m_camera_ubos_memory;
		std::vector<VkDescriptorSet> m_descriptor_sets;

		vk_wrapper::VulkanInstance m_instance;
		vk_wrapper::VulkanDebugMessenger m_debug_messenger;
		vk_wrapper::VulkanSwapchain m_swapchain;
		vk_wrapper::VulkanDevice m_device;
	};
}