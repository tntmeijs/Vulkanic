// Application
#include "vk_instance.hpp"
#include "vk_utility.hpp"

// Spdlog
#include <spdlog/spdlog.h>

using namespace vkc::vk_wrapper;

void VulkanInstance::Create(
	const std::string app_name,
	const std::string engine_name,
	uint32_t app_version_major,
	uint32_t app_version_minor,
	uint32_t app_version_patch,
	uint32_t engine_version_major,
	uint32_t engine_version_minor,
	uint32_t engine_version_patch,
	const std::vector<std::string>& extensions,
	const std::vector<std::string>& validation_layers) noexcept(false)
{
	if (extensions.empty())
	{
		// Most Vulkan applications use at least one extension
		spdlog::warn("No extensions specified, are you 100% sure this is intended?");
	}

	const auto app_version_number = VK_MAKE_VERSION(
		app_version_major,
		app_version_minor,
		app_version_patch);

	const auto engine_version_number = VK_MAKE_VERSION(
		engine_version_major,
		engine_version_minor,
		engine_version_patch);

	vk::ApplicationInfo app_info =
	{
		app_name.c_str(),		// Application name
		app_version_number,		// Application version
		engine_name.c_str(),	// Engine name
		engine_version_number,	// Engine version
		VK_API_VERSION_1_0		// Vulkan API version
	};

	std::vector<std::string> available_extension_names;
	std::vector<std::string> available_layer_names;

	// Configure extensions if necessary
	if (!extensions.empty())
	{
		auto available_extensions = vk::enumerateInstanceExtensionProperties();

		// Save the names of all available extensions
		available_extension_names.reserve(available_extensions.size());
		for (const auto& extension : available_extensions)
		{
			available_extension_names.push_back(extension.extensionName);
		}

		// Not every required extension is available
		if (!utility::AllRequiredItemsExistInVector(
			extensions,
			available_extension_names))
		{
			throw utility::CriticalVulkanError("A required extension is missing.");
		}
	}

	// Configure validation layers if necessary
	if (!validation_layers.empty())
	{
		auto available_validation_layers = vk::enumerateInstanceLayerProperties();

		// Save the names of all available validation layers
		available_layer_names.reserve(available_validation_layers.size());
		for (const auto& layer : available_validation_layers)
		{
			available_layer_names.push_back(layer.layerName);
		}

		// Not every validation layer is available
		if (!utility::AllRequiredItemsExistInVector(
			validation_layers,
			available_layer_names))
		{
			throw utility::CriticalVulkanError("A required validation layer is missing.");
		}
	}

	// The create info structure below needs a vector of c-strings
	const auto cstring_extensions = utility::ConvertVectorOfStringsToCString(extensions);
	const auto cstring_layers = utility::ConvertVectorOfStringsToCString(validation_layers);

	vk::InstanceCreateInfo instance_info = {};
	instance_info.pApplicationInfo = &app_info;
	instance_info.enabledExtensionCount = static_cast<uint32_t>(cstring_extensions.size());
	instance_info.ppEnabledExtensionNames = cstring_extensions.empty() ? nullptr : cstring_extensions.data();
	instance_info.enabledLayerCount = static_cast<uint32_t>(cstring_layers.size());
	instance_info.ppEnabledLayerNames = cstring_layers.empty() ? nullptr : cstring_layers.data();

	// Create the Vulkan instance
	m_instance = vk::createInstance(instance_info);
}

const vk::Instance& VulkanInstance::GetNative() const noexcept(true)
{
	return m_instance;
}

void VulkanInstance::Destroy() noexcept(true)
{
	m_instance.destroy();
}
