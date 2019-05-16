#ifndef VULKAN_SHADER_HPP
#define VULKAN_SHADER_HPP

// Vulkan
#include <vulkan/vulkan.h>

namespace vkc::vk_wrapper
{
	class VulkanShader
	{
	public:
		VulkanShader() {}
		~VulkanShader() {}

		enum class ShaderType
		{
			VertexShader,
			FragmentShader,
			ComputeShader
		};

	private:
	};
}

#endif
