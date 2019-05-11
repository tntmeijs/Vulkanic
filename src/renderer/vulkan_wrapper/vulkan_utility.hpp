#ifndef VK_UTILITY_HPP
#define VK_UTILITY_HPP

// C++ standard
#include <string>
#include <vector>

namespace vkc::vk_wrapper::utility
{
	/** Convert a vector of std::string to a vector of const char* */
	/**
	 * Vulkan still uses a C API under the hood, so sometimes it is required to
	 * pass an array of c-strings. This function makes the conversion easy.
	 */
	inline const std::vector<const char*> ConvertVectorOfStringsToCString(
		const std::vector<std::string>& original) noexcept(true)
	{
		std::vector<const char*> cstring_vector;

		for (const auto& str : original)
		{
			cstring_vector.push_back(str.c_str());
		}

		return cstring_vector;
	}

	/** Check if the required names exist in the "list" of all names */
	inline bool AllRequiredItemsExistInVector(
		const std::vector<std::string>& required_names,
		const std::vector<std::string>& all_names) noexcept(true)
	{
		if (all_names.empty())
		{
			// No required names available
			return false;
		}
		else if (required_names.empty())
		{
			// No required names given
			return true;
		}

		// Check if all requested names are available
		for (const auto& required_name : required_names)
		{
			bool found_required_name = false;

			for (const auto& name : all_names)
			{
				if (required_name == name)
				{
					found_required_name = true;
				}
			}

			// Could not find the specified name
			if (!found_required_name)
			{
				return false;
			}
		}

		return true;
	}
}

#endif
