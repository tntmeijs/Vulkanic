#include "viewport.hpp"

using namespace vkc::core;

void Viewport::Create(
	std::uint32_t pos_x,
	std::uint32_t pos_y,
	std::uint32_t width,
	std::uint32_t height) noexcept(true)
{
	SetPosX(pos_x);
	SetPosY(pos_y);
	SetWidth(width);
	SetHeight(height);
}

void Viewport::SetPosX(std::uint32_t pos_x) noexcept(true)
{
	m_pos_x = pos_x;
}

void Viewport::SetPosY(std::uint32_t pos_y) noexcept(true)
{
	m_pos_y = pos_y;
}

void Viewport::SetWidth(std::uint32_t width) noexcept(true)
{
	m_width = width;
}

void Viewport::SetHeight(std::uint32_t height) noexcept(true)
{
	m_height = height;
}

const std::uint32_t Viewport::GetPosX() const noexcept(true)
{
	return m_pos_x;
}

const std::uint32_t Viewport::GetPosY() const noexcept(true)
{
	return m_pos_y;
}

const std::uint32_t Viewport::GetWidth() const noexcept(true)
{
	return m_width;
}

const std::uint32_t Viewport::GetHeight() const noexcept(true)
{
	return m_height;
}

const float Viewport::GetAspectRatio() const noexcept(true)
{
	return (static_cast<float>(m_width) / static_cast<float>(m_height));
}
