#ifndef VIEWPORT_HPP
#define VIEWPORT_HPP

// C++ standard
#include <cstdint>

namespace vkc::core
{
	class Viewport
	{
	public:
		Viewport() noexcept(true)
			: m_pos_x(0)
			, m_pos_y(0)
			, m_width(0)
			, m_height(0)
		{}
		~Viewport() noexcept(true) {}

		/** Create a new viewport using the specified positions and dimensions */
		void Create(
			std::uint32_t pos_x,
			std::uint32_t pos_y,
			std::uint32_t width,
			std::uint32_t height) noexcept(true);

		/** Set the horizontal offset of the viewport */
		void SetPosX(std::uint32_t pos_x) noexcept(true);

		/** Set the vertical offset of the viewport */
		void SetPosY(std::uint32_t pos_y) noexcept(true);

		/** Set the width of the viewport */
		void SetWidth(std::uint32_t width) noexcept(true);

		/** Set the height of the viewport */
		void SetHeight(std::uint32_t height) noexcept(true);

		/** Get the horizontal offset of the viewport */
		const std::uint32_t GetPosX() const noexcept(true);

		/** Get the vertical offset of the viewport */
		const std::uint32_t GetPosY() const noexcept(true);

		/** Get the width of the viewport */
		const std::uint32_t GetWidth() const noexcept(true);

		/** Get the height of the viewport */
		const std::uint32_t GetHeight() const noexcept(true);

		/** Get the aspect ratio of the viewport (width / height) */
		const float GetAspectRatio() const noexcept(true);

	private:
		std::uint32_t m_pos_x;
		std::uint32_t m_pos_y;
		std::uint32_t m_width;
		std::uint32_t m_height;
	};
}

#endif
