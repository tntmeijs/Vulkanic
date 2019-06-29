#ifndef MEMORY_MANAGER_HPP
#define MEMORY_MANAGER_HPP

// Vulkan
#include <vulkan/vulkan.h>

// VulkanMemoryAllocator
#include <vk_mem_alloc.h>

// C++ standard
#include <vector>

namespace vkc
{
	namespace vk_wrapper
	{
		class VulkanDevice;
	}

	namespace memory
	{
		/** Wraps various allocation information objects for buffers */
		struct BufferAllocationInfo
		{
			VkBufferCreateInfo buffer_create_info = {};
			VmaAllocationCreateInfo allocation_info = {};
		};

		/** Wraps various allocation information objects for images */
		struct ImageAllocationInfo
		{
			VkImageCreateInfo image_create_info = {};
			VmaAllocationCreateInfo allocation_info = {};
		};

		/** Keeps the buffer object and its ID together */
		struct VulkanBuffer
		{
			VkBuffer buffer;
			VmaAllocationInfo info;
			VmaAllocation allocation;
			std::uint64_t id;
		};

		/** Keeps the image object and its ID together */
		struct VulkanImage
		{
			VkImage image;
			VmaAllocationInfo info;
			VmaAllocation allocation;
			std::uint64_t id;
		};

		/** Singleton! */
		class MemoryManager
		{
		public:
			/** Is not needed for a Singleton */
			MemoryManager(MemoryManager const&) = delete;

			/** Is not needed for a Singleton */
			void operator=(MemoryManager const&) = delete;

			/** Get hold of the Singleton instance */
			static MemoryManager& GetInstance();

			/** Initialize the memory manager */
			void Initialize(const vk_wrapper::VulkanDevice& device) noexcept(false);

			/** Destroy the memory allocator */
			void Destroy() noexcept(true);

			/** Free a previously allocated buffer */
			void Free(const VulkanBuffer& buffer) noexcept(false);

			/** Free a previously allocated image */
			void Free(const VulkanImage& image) noexcept(false);

			/** Map a buffer to a CPU pointer */
			/**
			 * Returns a pointer to which the buffer is mapped.
			 */
			void* MapBuffer(const VulkanBuffer& buffer);

			/** Unmap a buffer from a CPU pointer */
			void UnMapBuffer(const VulkanBuffer& buffer);

			/** Allocate a new buffer */
			const VulkanBuffer& Allocate(const BufferAllocationInfo& buffer_info) noexcept(false);

			/** Allocate a new image */
			const VulkanImage& Allocate(const ImageAllocationInfo& image_info) noexcept(false);

			/** Get a reference to the VulkanMemoryAllocator allocator object */
			const VmaAllocator& GetVMAAllocation() const noexcept(true);

		private:
			/** Is not needed for a Singleton */
			MemoryManager();

			/** Unique resource ID generator */
			static std::uint64_t CreateNewID() noexcept(true);

		private:
			/** VulkanMemoryAllocator allocator */
			VmaAllocator m_allocator;

			/** Container for all allocated buffers */
			std::vector<VulkanBuffer> m_buffers;

			/** Container for all allocated images */
			std::vector<VulkanImage> m_images;

			/** Flag that indicated whether "Initialize()" has already been called once */
			bool m_is_initialized;
		};
	}
}

#endif
