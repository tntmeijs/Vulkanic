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
	/** Simple vertex with a position, color, and texture coordinate */
	class VertexPCT
	{
	public:
		VertexPCT() noexcept(true);
		VertexPCT(const glm::vec3& position, const glm::vec3& color, const glm::vec2& texture_coordinate);
		~VertexPCT() noexcept(true);
		
		/** Get a list of input binding description structures needed to work with this vertex */
		static std::vector<VkVertexInputBindingDescription> GetBindingDescriptions() noexcept(true);

		/** Get a list of input attribute description structures needed to work with this vertex */
		static std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions() noexcept(true);

	public:
		glm::vec3 position;
		glm::vec3 color;
		glm::vec2 texture_coordinate;
	};
}

#endif // VERTEX_HPP
