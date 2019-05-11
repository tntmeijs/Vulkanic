// Application
#include "miscellaneous/exceptions.hpp"
#include "vulkan_instance.hpp"
#include "vulkan_utility.hpp"

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

	VkApplicationInfo app_info	= {};
	app_info.sType				= VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app_info.pApplicationName	= app_name.c_str();
	app_info.applicationVersion	= app_version_number;
	app_info.pEngineName		= engine_name.c_str();
	app_info.engineVersion		= engine_version_number;
	app_info.apiVersion			= VK_API_VERSION_1_0;

	std::vector<std::string> available_extension_names;
	std::vector<std::string> available_layer_names;

	// Configure extensions if necessary
	if (!extensions.empty())
	{
		std::uint32_t extension_count = 0;
		std::vector<VkExtensionProperties> available_extensions;
		vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);
		available_extensions.resize(extension_count);
		vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, available_extensions.data());

		// Save the names of all available extensions
		for (const auto& extension : available_extensions)
		{
			available_extension_names.push_back(extension.extensionName);
		}

		// Not every required extension is available
		if (!utility::AllRequiredItemsExistInVector(
			extensions,
			available_extension_names))
		{
			throw exception::CriticalVulkanError("A required extension is missing.");
		}
	}

	// Configure validation layers if necessary
	if (!validation_layers.empty())
	{
		std::uint32_t layer_count = 0;
		std::vector<VkLayerProperties> available_validation_layers;
		vkEnumerateInstanceLayerProperties(&layer_count, nullptr);
		available_validation_layers.resize(layer_count);
		vkEnumerateInstanceLayerProperties(&layer_count, available_validation_layers.data());

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
			throw exception::CriticalVulkanError("A required validation layer is missing.");
		}
	}

	// The create info structure below needs a vector of c-strings
	const auto cstring_extensions = utility::ConvertVectorOfStringsToCString(extensions);
	const auto cstring_layers = utility::ConvertVectorOfStringsToCString(validation_layers);

	VkInstanceCreateInfo instance_info = {};
	instance_info.pApplicationInfo = &app_info;
	instance_info.enabledExtensionCount = static_cast<uint32_t>(cstring_extensions.size());
	instance_info.ppEnabledExtensionNames = cstring_extensions.empty() ? nullptr : cstring_extensions.data();
	instance_info.enabledLayerCount = static_cast<uint32_t>(cstring_layers.size());
	instance_info.ppEnabledLayerNames = cstring_layers.empty() ? nullptr : cstring_layers.data();

	// Create the Vulkan instance
	auto result = vkCreateInstance(&instance_info, nullptr, &m_instance);

	if (result != VK_SUCCESS)
	{
		throw exception::CriticalVulkanError("Could not create an instance.");
	}
}

const VkInstance& VulkanInstance::GetNative() const noexcept(true)
{
	return m_instance;
}

void VulkanInstance::Destroy() noexcept(true)
{
	vkDestroyInstance(m_instance, nullptr);
}
