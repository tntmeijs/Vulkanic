#ifndef VULKAN_PIPELINE_HPP
#define VULKAN_PIPELINE_HPP

// Application
#include "vulkan_enums.hpp"

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

		namespace structs
		{
			struct VulkanPipelineInfo;
		}

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
				const structs::VulkanPipelineInfo* const pipeline_info,
				enums::PipelineType type,
				VkPipelineLayout layout,
				VkRenderPass render_pass,
				const std::vector<std::pair<std::string, enums::ShaderType>>& shader_files) noexcept(false);

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
				const structs::VulkanPipelineInfo* const pipeline_info) noexcept(false);

			/** Create a compute pipeline */
			void CreateComputePipeline(
				const structs::VulkanPipelineInfo* const pipeline_info) noexcept(false);
			
			/** Create a ray-tracing pipeline using VK_NV_raytracing */
			void CreateRayTracingPipeline(
				const structs::VulkanPipelineInfo* const pipeline_info) noexcept(false);

		private:
			VkPipeline m_pipeline;
		};
	}
}

#endif
