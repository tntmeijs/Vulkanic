#pragma once

// Application Vulkan wrappers
#include "memory_manager/memory_manager.hpp"
#include "vulkan_wrapper/vulkan_debug_messenger.hpp"
#include "vulkan_wrapper/vulkan_device.hpp"
#include "vulkan_wrapper/vulkan_instance.hpp"
#include "vulkan_wrapper/vulkan_pipeline.hpp"
#include "vulkan_wrapper/vulkan_render_pass.hpp"
#include "vulkan_wrapper/vulkan_swapchain.hpp"
#include "vulkan_wrapper/vulkan_command_buffer.hpp"
#include "vulkan_wrapper/vulkan_command_pool.hpp"
#include "vulkan_wrapper/vulkan_texture.hpp"
#include "vulkan_wrapper/vulkan_texture_sampler.hpp"
#include "vulkan_wrapper/vulkan_uniform_buffer.hpp"
#include "vulkan_wrapper/vulkan_vertex_buffer.hpp"
#include "memory_manager/memory_manager.hpp"

// Application core
#include "core/window.hpp"

//////////////////////////////////////////////////////////////////////////

// Spdlog (including it here to avoid the "APIENTRY": macro redefinition warning)
#include <spdlog/spdlog.h>

//////////////////////////////////////////////////////////////////////////

// C++ standard
#include <memory>
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
		void Destroy();

	private:
		void CreateGraphicsPipeline();
		void CreateFramebuffers();
		void RecordFrameCommands();
		void CreateSynchronizationObjects();
		void RecreateSwapchain(const Window& window);
		void CleanUpSwapchain();
		void CreateUniformBuffers();
		void CreateDescriptorPool();
		void CreateDescriptorSetLayout();
		void CreateDescriptorSets();
		
		static void CopyStagingBufferToDeviceLocalBuffer(
			const vk_wrapper::VulkanDevice& device,
			const memory::VulkanBuffer&  source,
			const memory::VulkanBuffer& destination,
			const VkQueue& queue,
			const vk_wrapper::VulkanCommandPool& pool);

	private:
		GLFWwindow* m_window;
		uint64_t m_frame_index;
		uint32_t m_current_swapchain_image_index;

		bool m_framebuffer_resized;

		VkDescriptorSetLayout m_camera_data_descriptor_set_layout;
		VkPipelineLayout m_pipeline_layout;
		VkDescriptorPool m_descriptor_pool;

		vk_wrapper::VulkanVertexBuffer m_vertex_buffer;
		std::vector<vk_wrapper::VulkanUniformBuffer> m_camera_ubos;

		std::vector<VkFramebuffer> m_swapchain_framebuffers;
		std::vector<VkSemaphore> m_in_flight_frame_image_available_semaphores;
		std::vector<VkSemaphore> m_in_flight_render_finished_semaphores;
		std::vector<VkFence> m_in_flight_fences;
		std::vector<VkDescriptorSet> m_descriptor_sets;

		vk_wrapper::VulkanInstance m_instance;
		vk_wrapper::VulkanDebugMessenger m_debug_messenger;
		vk_wrapper::VulkanSwapchain m_swapchain;
		vk_wrapper::VulkanDevice m_device;
		vk_wrapper::VulkanPipeline m_graphics_pipeline;
		vk_wrapper::VulkanRenderPass m_render_pass;
		vk_wrapper::VulkanCommandPool m_graphics_command_pool;
		vk_wrapper::VulkanCommandBuffer m_graphics_command_buffers;
		vk_wrapper::VulkanTexture m_uv_map_checker_texture;
		vk_wrapper::VulkanTextureSampler m_default_sampler;
	};
}