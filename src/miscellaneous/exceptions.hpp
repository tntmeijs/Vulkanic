#ifndef EXCEPTIONS_HPP
#define EXCEPTIONS_HPP

// C++ standard
#include <exception>
#include <string>

namespace vkc::exception
{
    /** Execution should be halted, critical error in Vulkan */
	/**
	 * When this exception is thrown, it means that a function call to the
	 * Vulkan API failed. The result of the function was critical for the entire
	 * application, which means that the execution cannot be continued without
	 * resolving the cause of this exception.
	 */
	class CriticalVulkanError : public std::exception
	{
	public:
		CriticalVulkanError(std::string message = "") : m_message(message) {}
		~CriticalVulkanError() = default;

		std::string what() { return m_message.c_str(); }

	private:
		std::string m_message;
	};

	/** Execution should be halted, critical error when reading from disc */
	/**
	 * When this exception is thrown, it means that an IO function call failed.
	 * This is most likely a failure that occured when reading from a file. The
	 * result of the function was critical for the entire application, which
	 * means that the execution cannot be continued without resolving the cause
	 * of this exception.
	 */
	class CriticalIOError : public std::exception
	{
	public:
		CriticalIOError(std::string message = "") : m_message(message) {}
		~CriticalIOError() = default;

		std::string what() { return m_message.c_str(); }

	private:
		std::string m_message;
	};

	/** Execution should be halted, critical error in the window */
	/**
	 * Somehow something went wrong with the window. This could be caused by
	 * various things in the window class. Be sure to check whether your
	 * computer is supported by this application (see: GitHub).
	 */
	class CriticalWindowError : public std::exception
	{
	public:
		CriticalWindowError(std::string message = "") : m_message(message) {}
		~CriticalWindowError() = default;

		std::string what() { return m_message.c_str(); }

	private:
		std::string m_message;
	};
}

#endif
