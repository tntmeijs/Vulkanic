#ifndef VULKAN_SHADER_HPP
#define VULKAN_SHADER_HPP

// Glslang (needs to be included before Vulkan header)
#include <glslang/Public/ShaderLang.h>

// Vulkan
#include <vulkan/vulkan.h>

// C++ standard
#include <string>
#include <vector>

namespace vkc::vk_wrapper
{
	class VulkanDevice;

	/** Types of shader supported by this application */
	enum class ShaderType
	{
		// Standard shader types
		Vertex = VK_SHADER_STAGE_VERTEX_BIT,
		Fragment = VK_SHADER_STAGE_FRAGMENT_BIT,
		Compute = VK_SHADER_STAGE_COMPUTE_BIT,

		// NVIDIA RTX shader types (extension, RTX only)
		RayGeneration_NV = VK_SHADER_STAGE_RAYGEN_BIT_NV,
		RayClosestHit_NV = VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV,
		RayMiss_NV = VK_SHADER_STAGE_MISS_BIT_NV,
		RayAnyHit_NV = VK_SHADER_STAGE_ANY_HIT_BIT_NV
	};

	class VulkanShader
	{
	public:
		VulkanShader() noexcept(true) {}
		~VulkanShader() noexcept(true) {}

		/** Create a Vulkan shader (includes loading / storing) */
		void Create(
			const VulkanDevice& device,
			const std::vector<std::pair<std::string, ShaderType>>& shader_files) noexcept(false);

		/** Destroy all Vulkan objects */
		void Destroy(const VulkanDevice& device) const noexcept(true);

		/** Get a reference to the pipeline shader stage create info vector */
		const std::vector<VkPipelineShaderStageCreateInfo>& GetPipelineShaderStageInfos() const noexcept(true);

	private:
		/** Load GLSL from file and convert to byte code */
		/**
		 * GLSL -> SPIRV referenced from: https://forestsharp.com/glslang-cpp/
		 * Shader includes are supported as long as each shader file has the
		 * following code at the top:
		 *
		 * "#extension GL_GOOGLE_include_directive : enable"
		 */
		std::vector<std::uint32_t> GetSPIRV(
			const std::string& path) const noexcept(false);

		/** Create a shader module out of shader bytecode */
		VkShaderModule CreateShaderModule(
			const VulkanDevice& device,
			const std::vector<std::uint32_t>& bytecode) const noexcept(false);

		/** Return the correct GLslang shader type based on file extension */
		EShLanguage GetShaderStageType(
			const std::string& path) const noexcept(false);

	private:
		std::vector<VkShaderModule> m_shader_modules;
		std::vector<VkPipelineShaderStageCreateInfo> m_shader_stage_infos;

		/** Glslang only needs to be initialized once in the application */
		static inline bool glsl_lang_initialized = false;
	};
}

#endif
