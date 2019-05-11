// Application
#include "miscellaneous/exceptions.hpp"
#include "window.hpp"

// C++ standard
#include <chrono>

using namespace vkc;
using namespace std::chrono;

void Window::Create(
	std::uint32_t initial_width,
	std::uint32_t initial_height,
	std::string title) noexcept(false)
{
	if (!glfwInit())
	{
		throw exception::CriticalWindowError("Could not initialize GLFW.");
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	// Create the window
	m_window_handle = glfwCreateWindow(
		initial_width,
		initial_height,
		title.c_str(),
		nullptr,
		nullptr);

	// Check whether the window was created successfully
	if (!m_window_handle)
	{
		Destroy();
		throw exception::CriticalWindowError("Could not create a window.");
	}

	// Save the "this" pointer to be able to use it in the callbacks
	glfwSetWindowUserPointer(m_window_handle, this);

	// Register callbacks
	glfwSetKeyCallback(m_window_handle, OnKeyInput);
	glfwSetFramebufferSizeCallback(m_window_handle, OnFramebufferResize);
}

void Window::Destroy() const noexcept(true)
{
	glfwDestroyWindow(m_window_handle);
	glfwTerminate();
}

GLFWwindow* Window::GetNative() const noexcept(true)
{
	return m_window_handle;
}

void Window::Stop() const noexcept(true)
{
	glfwSetWindowShouldClose(m_window_handle, GLFW_TRUE);
}

void Window::OnKey(
	std::function<void(int key, int action)> callback) noexcept(true)
{
	m_key_callback = callback;
}

void Window::OnResize(
	std::function<void(int new_width, int new_height)> callback) noexcept(true)
{
	m_resize_callback = callback;
}

void Window::OnInitialization(std::function<void()> callback) noexcept(true)
{
	m_initialization_callback = callback;
}

void Window::OnUpdate(
	std::function<void(double delta_time)> callback) noexcept(true)
{
	m_update_callback = callback;
}

void Window::OnDraw(
	std::function<void()> callback) noexcept(true)
{
	m_draw_callback = callback;
}

void Window::OnShutDown(std::function<void()> callback) noexcept(true)
{
	m_shut_down_callback = callback;
}

void Window::EnterMainLoop() const noexcept(true)
{
	// Initialize
	if (m_initialization_callback)
	{
		m_initialization_callback();
	}

	auto old_time = high_resolution_clock::now();

	while (!glfwWindowShouldClose(m_window_handle))
	{
		// Check for input
		PollInput();

		// Timestep calculations
		auto new_time = high_resolution_clock::now();
		auto delta_time = static_cast<double>((new_time - old_time).count());
		old_time = new_time;

		// Update
		if (m_update_callback)
		{
			m_update_callback(delta_time);
		}

		// Render
		if (m_draw_callback)
		{
			m_draw_callback();
		}
	}

	// Shut-down
	if (m_shut_down_callback)
	{
		m_shut_down_callback();
	}
}

void Window::PollInput() const noexcept(true)
{
	glfwPollEvents();
}

void Window::OnKeyInput(
	GLFWwindow* window,
	int key,
	int scancode,
	int action,
	int mods) noexcept(true)
{
	// Prevent "unreferenced formal parameter" warning from triggering
	scancode, mods;

	// Get hold of the stored window class pointer
	auto window_ptr = static_cast<Window*>(glfwGetWindowUserPointer(window));

	if (!window_ptr)
	{
		// Window pointer is not set
		return;
	}

	if (!window_ptr->m_key_callback)
	{
		// Key callback is not set
		return;
	}

	// Fire the key callback
	window_ptr->m_key_callback(key, action);
}

void Window::OnFramebufferResize(
	GLFWwindow* window,
	int new_width,
	int new_height) noexcept(true)
{
	// Get hold of the stored window class pointer
	auto window_ptr = static_cast<Window*>(glfwGetWindowUserPointer(window));

	if (!window_ptr)
	{
		// Window pointer is not set
		return;
	}

	if (!window_ptr->m_resize_callback)
	{
		// Resize callback is not set
		return;
	}

	// Fire the resize callback
	window_ptr->m_resize_callback(new_width, new_height);
}
