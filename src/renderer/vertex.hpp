#ifndef VERTEX_HPP
#define VERTEX_HPP

// Vulkan
#include <vulkan/vulkan.h>

// C++ standard
#include <vector>

namespace vkc
{
	/** Pure abstract class, all vertex types should inherit from this class */
    class VertexBase
    {
	public:
		VertexBase() noexcept(true) = default;
		~VertexBase() noexcept(true) = default;

		virtual const std::vector<VkVertexInputBindingDescription>& GetBindingDescriptions() const noexcept(true) = 0;
		virtual const std::vector<VkVertexInputAttributeDescription>& GetAttributeDescriptions() const noexcept(true) = 0;

	protected:
		std::vector<VkVertexInputBindingDescription> m_vertex_binding_descriptions;
		std::vector<VkVertexInputAttributeDescription> m_vertex_attribute_descriptions;
    };
}

#endif // VERTEX_HPP
