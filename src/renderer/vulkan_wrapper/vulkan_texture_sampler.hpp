#ifndef VULKAN_TEXTURE_SAMPLER_HPP
#define VULKAN_TEXTURE_SAMPLER_HPP

// Vulkan
#include <vulkan/vulkan.h>

namespace vkc::vk_wrapper
{
	// Forward declarations
	class VulkanDevice;

	/** Filter types used for texture lookups */
	enum class SamplerFilterType
	{
		Nearest	= VK_FILTER_NEAREST,
		Linear	= VK_FILTER_LINEAR,
		Cubic	= VK_FILTER_CUBIC_EXT
	};

	/** Dictates how a sampler should sample texels from a texture */
	enum class SamplingBehavior
	{
		Repeat				= VK_SAMPLER_ADDRESS_MODE_REPEAT,
		MirroredRepeat		= VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
		ClampToEdge			= VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		MirroredClampToEdge	= VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE,
		ClampToBorder		= VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER
	};

	/** Mipmap mode */
	enum class MipmapMode
	{
		Nearest	= VK_SAMPLER_MIPMAP_MODE_NEAREST,
		Linear	= VK_SAMPLER_MIPMAP_MODE_LINEAR
	};

	/** Compare operation used when sampling */
	enum class SamplerCompareOperation
	{
		Never			= VK_COMPARE_OP_NEVER,
		Less			= VK_COMPARE_OP_LESS,
		Equal			= VK_COMPARE_OP_EQUAL,
		LessEqual		= VK_COMPARE_OP_LESS_OR_EQUAL,
		Greater			= VK_COMPARE_OP_GREATER,
		NotEqual		= VK_COMPARE_OP_NOT_EQUAL,
		GreaterEqual	= VK_COMPARE_OP_GREATER_OR_EQUAL,
		Always			= VK_COMPARE_OP_ALWAYS
	};

	/** Container used to store the settings that will be used to create a sampler */
	/**
	 * The following settings are set by default, change them as you see fit:
	 *
	 *		- min_filter = SamplerFilterType::Linear
	 *		- mag_filter = SamplerFilterType::Linear
	 *
	 *		- behavior_u = SamplingBehavior::ClampToBorder
	 *		- behavior_v = SamplingBehavior::ClampToBorder
	 *		- behavior_w = SamplingBehavior::ClampToBorder
	 *
	 *		- anisotropy_enabled = true
	 *		- anisotropy_value = 16.0f
	 *
	 *		- mipmap_mode = MipmapMode::Linear
	 *		- mipmap_lod_bias = 0.0f
	 *		- min_lod = 0.0f
	 *		- max_lod = 0.0f
	 *
	 *		- use_normalized_coordinates = true
	 *
	 *		- comparison_enabled = false
	 *		- compare_operation = SamplerCompareOperation::Always
	 */
	struct TextureSamplerSettings
	{
		SamplerFilterType min_filter = SamplerFilterType::Linear;
		SamplerFilterType mag_filter = SamplerFilterType::Linear;

		SamplingBehavior behavior_u = SamplingBehavior::ClampToBorder;
		SamplingBehavior behavior_v = SamplingBehavior::ClampToBorder;
		SamplingBehavior behavior_w = SamplingBehavior::ClampToBorder;

		bool anisotropy_enabled = true;
		float anisotropy_value = 16.0f;

		MipmapMode mipmap_mode = MipmapMode::Linear;
		float mipmap_lod_bias = 0.0f;
		float min_lod = 0.0f;
		float max_lod = 0.0f;

		bool use_normalized_coordinates = true;

		bool comparison_enabled = false;
		SamplerCompareOperation compare_operation = SamplerCompareOperation::Always;
	};

	/** Wrapper class that handles texture sampler creation */
    class VulkanTextureSampler
    {
    public:
        VulkanTextureSampler() noexcept(true);
        ~VulkanTextureSampler() noexcept(true);

		/** Create a new Vulkan sampler using the default sampler settings */
		void Create(const VulkanDevice& device) noexcept(true);

		/** Create a new Vulkan sampler object using the specified sampler settings */
		void Create(const VulkanDevice& device, const TextureSamplerSettings& settings) noexcept(false);

		/** Destroy the underlaying Vulkan object */
		void Destroy(const VulkanDevice& device) const noexcept(true);

		/** Get a reference to the Vulkan sampler object */
		const VkSampler& GetNative() const noexcept(true);

    private:
		VkSampler m_sampler;
    };
}

#endif // VULKAN_TEXTURE_SAMPLER
