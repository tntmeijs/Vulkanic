#pragma once

//////////////////////////////////////////////////////////////////////////

// C++ standard
#include <array>
#include <string>
#include <vector>

//////////////////////////////////////////////////////////////////////////

namespace vkc::global_settings
{
	//////////////////////////////////////////////////////////////////////////
	// Window dimensions, application name, engine name, window title, etc.
	//////////////////////////////////////////////////////////////////////////

	static const constexpr std::uint32_t default_window_width	= 2560;
	static const constexpr std::uint32_t default_window_height	= 1440;

	static const constexpr char* window_title		= "Vulkanic Renderer | Tahar Meijs";
	static const constexpr char* application_name	= "Vulkanic Renderer";
	static const constexpr char* engine_name		= "Vulkanic";

	static const constexpr std::uint32_t application_version[3]	= { 1, 0, 0 };
	static const constexpr std::uint32_t engine_version[3]		= { 1, 0, 0 };

	static const constexpr std::uint32_t maximum_in_flight_frame_count = 2;

	//////////////////////////////////////////////////////////////////////////
	// Vulkan validation layers
	//////////////////////////////////////////////////////////////////////////
	static const std::vector<std::string> validation_layer_names =
	{
		"VK_LAYER_LUNARG_standard_validation"
	};

	//////////////////////////////////////////////////////////////////////////
	// Vulkan instance extensions
	//////////////////////////////////////////////////////////////////////////
	static const constexpr std::array<const char*, 0> instance_extension_names =
	{
		// ADD ADDITIONAL REQUIRED EXTENSION NAMES HERE
	};

	//////////////////////////////////////////////////////////////////////////
	// Vulkan device extensions
	//////////////////////////////////////////////////////////////////////////
	static const std::vector<std::string> device_extension_names =
	{
		// ADD ADDITIONAL REQUIRED EXTENSION NAMES HERE
		"VK_KHR_swapchain"
	};
}