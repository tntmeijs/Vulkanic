#pragma once

// Application Vulkan wrappers
#include "memory_manager/memory_manager.hpp"
#include "vulkan_wrapper/vulkan_debug_messenger.hpp"
#include "vulkan_wrapper/vulkan_device.hpp"
#include "vulkan_wrapper/vulkan_instance.hpp"
#include "vulkan_wrapper/vulkan_pipeline.hpp"
#include "vulkan_wrapper/vulkan_render_pass.hpp"
#include "vulkan_wrapper/vulkan_swapchain.hpp"
#include "memory_manager/virtual_buffer.hpp"
#include "vulkan_wrapper/vulkan_command_buffer.hpp"
#include "vulkan_wrapper/vulkan_command_pool.hpp"

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
	class memory::VirtualBuffer;

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
		void CreateFramebuffers();
		void CreateTextureImage();
		void CreateTextureImageView();
		void CreateTextureSampler();
		void RecordFrameCommands();
		void CreateSynchronizationObjects();
		void RecreateSwapchain(const Window& window);
		void CleanUpSwapchain();
		void CreateVertexBuffer();
		void CreateUniformBuffers();
		void CreateDescriptorPool();
		void CreateDescriptorSetLayout();
		void CreateDescriptorSets();
		
		static void CopyStagingBufferToDeviceLocalBuffer(
			const vk_wrapper::VulkanDevice& device,
			const memory::VirtualBuffer* const  source,
			const memory::VirtualBuffer* const destination,
			const VkQueue& queue,
			const VkCommandPool pool);

		static VkCommandBuffer BeginSingleTimeCommands(
			const VkCommandPool& pool,
			const vk_wrapper::VulkanDevice& device);

		static void EndSingleTimeCommands(
			const vk_wrapper::VulkanDevice& device,
			const VkCommandPool& pool,
			const VkCommandBuffer& cmd_buffer,
			const VkQueue& queue);

		static void TransitionImageLayout(
			const vk_wrapper::VulkanDevice& device,
			const VkCommandPool& pool,
			const VkQueue& queue,
			const VkImage& image,
			const VkImageLayout& current_layout,
			const VkImageLayout& new_layout);

		static void CopyBufferToImage(
			const vk_wrapper::VulkanDevice& device,
			const VkCommandPool& pool,
			const VkQueue& queue,
			const VkBuffer& buffer,
			const VkImage& image,
			std::uint32_t width,
			std::uint32_t height);

	private:
		GLFWwindow* m_window;
		uint64_t m_frame_index;
		uint32_t m_current_swapchain_image_index;

		bool m_framebuffer_resized;

		VkDescriptorSetLayout m_camera_data_descriptor_set_layout;
		VkPipelineLayout m_pipeline_layout;
		VkDescriptorPool m_descriptor_pool;
		VkImage m_texture_image;
		VkDeviceMemory m_texture_image_memory;
		VkImageView m_texture_image_view;
		VkSampler m_texture_sampler;

		std::unique_ptr<memory::VirtualBuffer> m_vertex_buffer;
		std::vector<memory::VirtualBuffer> m_camera_ubos;

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

		memory::MemoryManager m_memory_manager;
	};
}