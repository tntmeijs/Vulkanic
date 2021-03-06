#ifndef VULKAN_PIPELINE_HPP
#define VULKAN_PIPELINE_HPP

// Application
#include "vulkan_pipeline_info.hpp"
#include "vulkan_shader.hpp"

// Vulkan
#include <vulkan/vulkan.h>

// C++ standard
#include <string>
#include <vector>

namespace vkc
{
	namespace core
	{
		class Viewport;
	}

	namespace vk_wrapper
	{
		class VulkanDevice;
		class VulkanShader;

		/** Pipelines supported by this application */
		enum class PipelineType
		{
			// Standard pipeline types
			Graphics,
			Compute,

			// NVIDIA RTX pipeline type (extension, RTX only)
			RayTracing_NV
		};

		/** Handles Vulkan pipeline creation for the graphics, compute, and ray-tracing pipeline */
		class VulkanPipeline
		{
		public:
			VulkanPipeline() noexcept(true) : m_pipeline(VK_NULL_HANDLE) {}
			~VulkanPipeline() noexcept(true) {}

			/** Create a Vulkan pipeline */
			/**
			 * The "VulkanPipelinInfo" structure should be a pointer to a child
			 * structure. Internally the structure is casted to a child class of
			 * the correct type. Make sure the "VulkanPipelineInfo" passed to
			 * this function is compatible with the specified "PipelineType".
			 */
			void Create(
				const VulkanDevice& device,
				const VulkanPipelineInfo* const pipeline_info,
				PipelineType type,
				VkPipelineLayout layout,
				VkRenderPass render_pass,
				const std::vector<std::pair<std::string, ShaderType>>& shader_files) noexcept(false);

			/** Get a reference to the underlaying Vulkan pipeline object */
			const VkPipeline& GetNative() const noexcept(true);

			/** Destroy Vulkan resources */
			void Destroy(const VulkanDevice& device) const noexcept(true);

		private:
			/** Create a graphics pipeline */
			void CreateGraphicsPipeline(
				VkPipelineLayout layout,
				VkRenderPass render_pass,
				const VulkanDevice& device,
				const VulkanShader& shader,
				const VulkanPipelineInfo* const pipeline_info) noexcept(false);

			/** Create a compute pipeline */
			void CreateComputePipeline(
				const VulkanPipelineInfo* const pipeline_info) noexcept(false);
			
			/** Create a ray-tracing pipeline using VK_NV_raytracing */
			void CreateRayTracingPipeline(
				const VulkanPipelineInfo* const pipeline_info) noexcept(false);

		private:
			VkPipeline m_pipeline;
		};
	}
}

#endif
