#ifndef WINDOW_HPP
#define WINDOW_HPP

// GLFW
#include <GLFW/glfw3.h>

// C++ standard
#include <functional>
#include <string>

namespace vkc
{
	class Window
	{
	public:
		Window() noexcept(true) : m_window_handle(nullptr) {}
		~Window() noexcept(true) {}

		/** Initialize GLFW, create a window, and register callbacks */
		void Create(
			std::uint32_t initial_width,
			std::uint32_t initial_height,
			std::string title) noexcept(false);

		/** Get a handle to the GLFW window */
		GLFWwindow* GetNative() const noexcept(true);

		/** Close the window (stop the main loop) */
		void Stop() const noexcept(true);

		/** Register a key callback */
		void OnKey(std::function<void(int, int)> callback) noexcept(true);

		/** Register a window resize callback */
		void OnResize(std::function<void(int, int)> callback) noexcept(true);

		/** Register the initialization callback function */
		/**
		 * All initialization should be performed when this callback is called.
		 */
		void OnInitialization(std::function<void()> callback) noexcept(true);

		/** Register the update callback function */
		/**
		 * All updates should be performed when this callback is called.
		 */
		void OnUpdate(std::function<void(double)> callback) noexcept(true);

		/** Register the draw callback function */
		/**
		 * All rendering should be performed when this callback is called.
		 */
		void OnDraw(std::function<void()> callback) noexcept(true);

		/** Register the shut-down callback function */
		/**
		 * All clean-up, deallocation, and shut-down procedures should be
		 * performed when this callback is called.
		 */
		void OnShutDown(std::function<void()> callback) noexcept(true);

		/** Update loop, calls the initialize, update, draw, and shut-down callbacks */
		/**
		 * The application main loop starts running when this function is called.
		 * The initialization callback is called first. After that finishes, the
		 * application enter an update/draw loop that keeps on going until the
		 * window "Stop" function is called, after which the shut-down callback
		 * is called.
		 */
		void EnterMainLoop() const noexcept(true);

		/** Check for input during this frame */
		void PollInput() const noexcept(true);

	private:
		/** Key callback function that calls the key callback member */
		/**
		 * It is impossible to register a member function as a key callback
		 * directly. This is why a private static function has to be used. In
		 * this function, the user pointer to the window class is retrieved,
		 * which allows the function to call the key callback member function.
		 *
		 * This function basically just redirects the callback to the key
		 * callback member function.
		 */
		static void OnKeyInput(
			GLFWwindow* window,
			int key,
			int scancode,
			int action,
			int mods) noexcept(true);

		/** Resize callback function that calls the resize callback member */
		/**
		 * It is impossible to register a member function as a resize callback
		 * directly. This is why a private static function has to be used. In
		 * this function, the user pointer to the window class is retrieved,
		 * which allows the function to call the resize callback member function.
		 *
		 * This function basically just redirects the callback to the resize
		 * callback member function.
		 */
		static void OnFramebufferResize(
			GLFWwindow* window,
			int new_width,
			int new_height) noexcept(true);

	private:
		GLFWwindow* m_window_handle;

		std::function<void()> m_draw_callback;
		std::function<void()> m_initialization_callback;
		std::function<void()> m_shut_down_callback;
		std::function<void(double delta_time)> m_update_callback;
		std::function<void(int key, int action)> m_key_callback;
		std::function<void(int new_width, int new_height)> m_resize_callback;
	};
}

#endif
