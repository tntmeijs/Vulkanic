#ifndef VULKAN_COMMAND_BUFFER_HPP
#define VULKAN_COMMAND_BUFFER_HPP

// Vulkan
#include <vulkan/vulkan.h>

// C++ standard
#include <vector>

namespace vkc::vk_wrapper
{
	class VulkanCommandPool;
	class VulkanDevice;

	enum class CommandBufferUsage
	{
		OneTimeSubmit = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
		SimultaneousUse = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
		SecondaryCommandBuffer = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT
	};

	class VulkanCommandBuffer
	{
	public:
		VulkanCommandBuffer() noexcept(true);
		~VulkanCommandBuffer() noexcept(true);

		/** Create one or multiple command buffers */
		void Create(
			const VulkanDevice& device,
			const VulkanCommandPool& command_pool,
			std::uint32_t command_buffer_count,
			bool is_primary = true) noexcept(false);

		/** Deallocate used command buffers */
		void Destroy(const VulkanDevice& device, const VulkanCommandPool& command_pool) const noexcept(true);

		/** Get the first Vulkan command buffer object in the command buffer vector */
		const VkCommandBuffer& GetNative() const noexcept(false);

		/** Get the Vulkan command buffer object from the command buffer vector at the specified index */
		/**
		 * Be careful, no bounds-check is performed when passing an index, this can trigger an out of range exception!
		 */
		const VkCommandBuffer& GetNative(std::uint32_t index) const noexcept(false);

		/** Start recording on the first command buffer in the vector */
		void BeginRecording(CommandBufferUsage usage) const noexcept(false);

		/** Start recording on the specified command buffer */
		/**
		 * Be careful, no bounds-check is performed when passing an index, this can trigger an out of range exception!
		 */
		void BeginRecording(std::uint32_t index, CommandBufferUsage usage) const noexcept(false);

		/** Stop recording on the first command buffer in the vector */
		void StopRecording() const noexcept(false);

		/** Stop recording on the specified command buffer */
		/**
		 * Be careful, no bounds-check is performed when passing an index, this can trigger an out of range exception!
		 */
		void StopRecording(std::uint32_t index) const noexcept(false);

	private:
		std::vector<VkCommandBuffer> m_command_buffers;
	};
}

#endif
