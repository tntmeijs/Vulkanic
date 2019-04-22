#pragma once

//////////////////////////////////////////////////////////////////////////

// C++ standard
#include <array>
#include <cstdint>

//////////////////////////////////////////////////////////////////////////

namespace vkc::global_settings
{
	//////////////////////////////////////////////////////////////////////////
	// Window dimensions, application name, engine name, window title, version
	//////////////////////////////////////////////////////////////////////////

	static const constexpr uint32_t default_window_width	= 1280;
	static const constexpr uint32_t default_window_height	= 720;

	static const constexpr char* window_title		= "Vulkanic Renderer | Tahar Meijs";
	static const constexpr char* application_name	= "Vulkanic Renderer";
	static const constexpr char* engine_name		= "Vulkanic";

	static const constexpr uint32_t application_version[3]	= { 1, 0, 0 };
	static const constexpr uint32_t engine_version[3]		= { 1, 0, 0 };

	//////////////////////////////////////////////////////////////////////////
	// Vulkan validation layers
	//////////////////////////////////////////////////////////////////////////
	static const constexpr std::array<const char*, 1> validation_layer_names =
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
	// VUlkan logical device extensions
	//////////////////////////////////////////////////////////////////////////
	static const constexpr std::array<const char*, 0> logical_device_extension_names =
	{
		// ADD ADDITIONAL REQUIRED EXTENSION NAMES HERE
	};
}