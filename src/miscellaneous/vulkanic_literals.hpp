#ifndef VULKANIC_LITERALS_HPP
#define VULKANIC_LITERALS_HPP

// C++ standard
#include <cstddef>

namespace vkc
{
	/** Convert Bytes to KiloBytes */
	constexpr auto operator"" _KB(std::size_t bytes) { return bytes * 1024; }
	
	/** Convert Bytes to MegaBytes */
	constexpr auto operator"" _MB(std::size_t bytes) { return bytes * 1024 * 1024; }
}

#endif
