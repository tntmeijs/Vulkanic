#ifndef VK_INSTANCE_HPP
#define VK_INSTANCE_HPP

// Vulkan
#include <vulkan/vulkan.hpp>

// C++ standard library
#include <string>
#include <vector>

namespace vkc::vk_wrapper
{
	class VulkanInstance
	{
	public:
		VulkanInstance() noexcept(true) {}
		~VulkanInstance() noexcept(true) {}

		/** Create a Vulkan instance */
		/**
		 * If the "validation_layers" vector is empty, the application will not
		 * provide the user with any kind of debugging information. Not even a
		 * console output log.
		 *
		 * This function throws a "CriticalVulkanError" exception when the
		 * application fails to properly create a Vulkan instance.
		 */
		void Create(
			const std::string app_name,
			const std::string engine_name,
			uint32_t app_version_major,
			uint32_t app_version_minor,
			uint32_t app_version_patch,
			uint32_t engine_version_major,
			uint32_t engine_version_minor,
			uint32_t engine_version_patch,
			const std::vector<std::string>& extensions,
			const std::vector<std::string>& validation_layers) noexcept(false);

		/** Get a reference to the Vulkan object */
		const vk::Instance& GetNative() const noexcept(true);

		/** Destroy the Vulkan instance */
		void Destroy() noexcept(true);

	private:
		vk::Instance m_instance;
	};
}

#endif
