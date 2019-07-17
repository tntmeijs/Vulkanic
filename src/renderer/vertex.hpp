#ifndef VERTEX_HPP
#define VERTEX_HPP

// Vulkan
#include <vulkan/vulkan.h>

// GLM
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

// C++ standard
#include <vector>

namespace vkc
{
	/** Pure abstract class, all vertex types should inherit from this class */
    class VertexBase
    {
	public:
		// TODO: add comments
		const std::vector<VkVertexInputBindingDescription>& GetBindingDescriptions() const noexcept(true);
		const std::vector<VkVertexInputAttributeDescription>& GetAttributeDescriptions() const noexcept(true);

	protected:
		VertexBase() noexcept(true) = default;
		~VertexBase() noexcept(true) = default;

	protected:
		std::vector<VkVertexInputBindingDescription> m_vertex_binding_descriptions;
		std::vector<VkVertexInputAttributeDescription> m_vertex_attribute_descriptions;
    };

	/** Simple vertex with a position, color, and texture coordinate */
	class VertexPCT final : public VertexBase
	{
	public:
		VertexPCT() noexcept(true);
		~VertexPCT() noexcept(true);
		
	public:
		glm::vec3 position;
		glm::vec3 color;
		glm::vec2 texture_coordinate;
	};
}

#endif // VERTEX_HPP
