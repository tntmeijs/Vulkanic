#ifndef VULKAN_TEXTURE_HPP
#define VULKAN_TEXTURE_HPP

// Vulkan
#include <vulkan/vulkan.h>

// C++ standard
#include <memory>
#include <string_view>

namespace vkc
{
	namespace memory
	{
		struct VulkanBuffer;
		struct VulkanImage;
	}

	namespace vk_wrapper
	{
		class VulkanCommandPool;
		class VulkanDevice;

		/**
		 * Class used to wrap all image-related Vulkan calls.
		 * Things such as image views, memory management, etc. are all taken care of.
		 */
		class VulkanTexture
		{
		public:
			VulkanTexture();
			~VulkanTexture();

			/** Create a Vulkan texture from the specified image file */
			void Create(
				const std::string_view path,
				VkFormat format,
				const VulkanDevice& device,
				const VulkanCommandPool& command_pool) noexcept(false);

			/** Destroy allocated resources */
			void Destroy(const VulkanDevice& device);

			/** Get a reference to the image backing this texture */
			const memory::VulkanImage& GetImage() const noexcept(true);

			/** Get a reference to the image view backing this texture */
			const VkImageView& GetImageView() const noexcept(true);

		private:
			/** Load the pixel data from the specified file, will throw when the file cannot be read from */
			unsigned char* LoadDataFromFile(const std::string_view path) noexcept(false);

			/** Create a staging buffer to use it to upload the texture to device memory */
			const memory::VulkanBuffer& CreateStagingBuffer(const VkDeviceSize buffer_size) noexcept(true);

			/** Create a Vulkan image object */
			void CreateImage() noexcept(true);

			/** Copy the staging buffer to the image device memory */
			void CopyStagingBufferToDeviceLocal(
				const memory::VulkanBuffer& staging_buffer,
				const VulkanDevice& device,
				const VulkanCommandPool& command_pool);

			/** Create an image view for this image */
			void CreateImageView(const VulkanDevice& device) noexcept(false);

		private:
			int m_width;
			int m_height;
			int m_channel_count;

			VkFormat m_format;
			VkImageView m_image_view;

			std::unique_ptr<memory::VulkanImage> m_image;
		};
	}
}

#endif // VULKAN_TEXTURE_HPP
