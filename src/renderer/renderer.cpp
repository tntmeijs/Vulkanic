//////////////////////////////////////////////////////////////////////////

// Vulkanic
#include "miscellaneous/global_settings.hpp"
#include "renderer.hpp"
#include "vulkan_wrapper/vulkan_enums.hpp"
#include "vulkan_wrapper/vulkan_structures.hpp"

//////////////////////////////////////////////////////////////////////////

// GLM
#include "glm/gtc/matrix_transform.hpp"
#include "glm/mat4x4.hpp"
#include "glm/vec3.hpp"

//////////////////////////////////////////////////////////////////////////

// C++ standard
#include <algorithm>
#include <fstream>
#include <set>
#include <string>
#include <vector>

//////////////////////////////////////////////////////////////////////////

using namespace vkc;

// Hard-coded models
struct Vertex
{
	glm::vec3 position;
	glm::vec3 color;

	static std::vector<VkVertexInputBindingDescription> GetBindingDescriptions()
	{
		VkVertexInputBindingDescription desc = {};
		desc.binding = 0;
		desc.stride = sizeof(Vertex);
		desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return { desc };
	}

	static std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions()
	{
		std::vector<VkVertexInputAttributeDescription> attribs(2);

		attribs[0].binding = 0;
		attribs[0].location = 0;
		attribs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attribs[0].offset = offsetof(Vertex, position);

		attribs[1].binding = 0;
		attribs[1].location = 1;
		attribs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attribs[1].offset = offsetof(Vertex, color);

		return attribs;
	}
};

const std::vector<Vertex> vertices =
{
	// Position					// Color
	{ {  0.0f, -0.5f, 0.0f }, { 1.0f, 1.0f, 1.0f } },
	{ { -0.5f,  0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f } },
	{ {  0.5f,  0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f } }
};

struct CameraData
{
	glm::mat4 model_matrix;
	glm::mat4 view_matrix;
	glm::mat4 projection_matrix;
};

Renderer::Renderer()
	: m_frame_index(0)
	, m_current_swapchain_image_index(0)
	, m_framebuffer_resized(false)
{}

Renderer::~Renderer()
{
	// Wait until the GPU finishes the current operation before cleaning-up resources
	vkDeviceWaitIdle(m_device.GetLogicalDeviceNative());

	CleanUpSwapchain();

	vkDestroyDescriptorSetLayout(m_device.GetLogicalDeviceNative(), m_camera_data_descriptor_set_layout, nullptr);
	vkDestroyBuffer(m_device.GetLogicalDeviceNative(), m_vertex_buffer, nullptr);
	vkFreeMemory(m_device.GetLogicalDeviceNative(), m_vertex_buffer_memory, nullptr);

	for (auto index = 0; index < global_settings::maximum_in_flight_frame_count; ++index)
	{
		vkDestroySemaphore(m_device.GetLogicalDeviceNative(), m_in_flight_frame_image_available_semaphores[index], nullptr);
		vkDestroySemaphore(m_device.GetLogicalDeviceNative(), m_in_flight_render_finished_semaphores[index], nullptr);
	}

	for (auto index = 0; index < global_settings::maximum_in_flight_frame_count; ++index)
	{
		vkDestroyFence(m_device.GetLogicalDeviceNative(), m_in_flight_fences[index], nullptr);
	}
	
	vkDestroyCommandPool(m_device.GetLogicalDeviceNative(), m_graphics_command_pool, nullptr);
	
	m_device.Destroy();

#ifdef _DEBUG
	m_debug_messenger.Destroy(m_instance);
#endif

	m_swapchain.DestroySurface(m_instance);
	m_instance.Destroy();
}

void Renderer::Initialize(const Window& window)
{
	m_window = window.GetNative();

	// Add all extensions required by GLFW
	std::uint32_t glfw_extension_count = 0;
	auto glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
	std::vector<std::string> required_extensions(glfw_extensions, glfw_extensions + glfw_extension_count);

#ifdef _DEBUG
	// When running in debug mode, add the message callback extension to the list
	required_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

	// Add any additional extension names to the list specified in the global settings file
	required_extensions.insert(required_extensions.end(), global_settings::instance_extension_names.begin(), global_settings::instance_extension_names.end());

	m_instance.Create(
		global_settings::application_name,
		global_settings::engine_name,
		global_settings::application_version[0],
		global_settings::application_version[1],
		global_settings::application_version[2],
		global_settings::engine_version[0],
		global_settings::engine_version[1],
		global_settings::engine_version[2],
		required_extensions,
		global_settings::validation_layer_names);

#ifdef _DEBUG
	// Enable validation layer messenger in debug mode
	m_debug_messenger.Create(m_instance);
#endif

	// Create the swapchain surface
	m_swapchain.CreateSurface(m_instance, window);

	// Create the logical device (physical device is created as well internally)
	m_device.Create(m_instance, m_swapchain, global_settings::device_extension_names);

	// Create the swapchain (also creates all related objects such as image views)
	m_swapchain.Create(m_device, window);

	CreateRenderPass();
	CreateDescriptorSetLayout();
	CreateGraphicsPipeline();
	CreateFramebuffers();
	CreateCommandPools();
	CreateVertexBuffer();
	CreateUniformBuffers();
	CreateDescriptorPool();
	CreateDescriptorSets();
	CreateCommandBuffers();
	CreateSynchronizationObjects();
}

void Renderer::Draw(const Window& window)
{
	// Wait for the fence of the old frame to be completed
	vkWaitForFences(m_device.GetLogicalDeviceNative(), 1, &m_in_flight_fences[m_frame_index], VK_TRUE, std::numeric_limits<uint64_t>::max());
	
	// Retrieve an image from the swapchain for writing (wait indefinitely for the image to become available)
	auto result = vkAcquireNextImageKHR(
		m_device.GetLogicalDeviceNative(),
		m_swapchain.GetNative(),
		std::numeric_limits<uint64_t>::max(),
		m_in_flight_frame_image_available_semaphores[m_frame_index],
		VK_NULL_HANDLE,
		&m_current_swapchain_image_index);

	// Recreate the swapchain if the current swapchain has become incompatible with the surface
	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		RecreateSwapchain(window);
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		spdlog::error("Could not acquire a new swapchain image.");
	}

	// Wait on these semaphores before execution can start
	VkSemaphore wait_semaphores[] = { m_in_flight_frame_image_available_semaphores[m_frame_index] };

	// Signal these semaphores once execution finishes
	VkSemaphore signal_semaphores[] = { m_in_flight_render_finished_semaphores[m_frame_index] };

	// Wait in these stages of the pipeline on the semaphores
	VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

	VkSubmitInfo submit_info = {};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.waitSemaphoreCount = sizeof(wait_semaphores) / sizeof(wait_semaphores[0]);
	submit_info.pWaitSemaphores = wait_semaphores;
	submit_info.pWaitDstStageMask = wait_stages;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &m_command_buffers[m_current_swapchain_image_index];
	submit_info.signalSemaphoreCount = sizeof(signal_semaphores) / sizeof(signal_semaphores[0]);
	submit_info.pSignalSemaphores = signal_semaphores;

	// Fence completed, reset its state
	vkResetFences(m_device.GetLogicalDeviceNative(), 1, &m_in_flight_fences[m_frame_index]);

	// Submit the command queue
	if (vkQueueSubmit(m_device.GetQueueNativeOfType(vk_wrapper::enums::VulkanQueueType::Graphics), 1, &submit_info, m_in_flight_fences[m_frame_index]) != VK_SUCCESS)
	{
		spdlog::error("Could not submit the queue for frame #{}.", m_current_swapchain_image_index);
		return;
	}

	VkPresentInfoKHR present_info = {};
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.waitSemaphoreCount = sizeof(signal_semaphores) / sizeof(signal_semaphores[0]);
	present_info.pWaitSemaphores = signal_semaphores;
	present_info.swapchainCount = 1;
	present_info.pSwapchains = &m_swapchain.GetNative();
	present_info.pImageIndices = &m_current_swapchain_image_index;

	// Request to present an image to the swapchain
	result = vkQueuePresentKHR(m_device.GetQueueNativeOfType(vk_wrapper::enums::VulkanQueueType::Present), &present_info);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_framebuffer_resized)
	{
		spdlog::warn("Swapchain is not up-to-date anymore, recreating swapchain...");

		m_framebuffer_resized = false;
		RecreateSwapchain(window);
	}
	else if (result != VK_SUCCESS)
	{
		spdlog::error("Could not present the swapchain image.");
		return;
	}

	// Advance to the next frame
	m_frame_index = (m_frame_index + 1) % global_settings::maximum_in_flight_frame_count;
}

void Renderer::Update()
{
	static float rotate_amount = 0.0f;
	rotate_amount += 0.0001f;

	CameraData cam_data = {};
	cam_data.model_matrix = glm::rotate(glm::mat4(1.0f), rotate_amount * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	cam_data.view_matrix = glm::lookAt(glm::vec3(0.0f, 0.0f, -0.5f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	cam_data.projection_matrix = glm::perspective(
		90.0f,
		static_cast<float>(m_swapchain.GetExtent().width) / static_cast<float>(m_swapchain.GetExtent().height),
		0.1f,
		1000.0f);

	// GLM is for OpenGL, Vulkan uses an inverted Y-coordinate
	cam_data.projection_matrix[1][1] *= -1.0f;

	void* data = nullptr;
	vkMapMemory(m_device.GetLogicalDeviceNative(), m_camera_ubos_memory[m_current_swapchain_image_index], 0, sizeof(cam_data), 0, &data);
	memcpy(data, &cam_data, sizeof(cam_data));
	vkUnmapMemory(m_device.GetLogicalDeviceNative(), m_camera_ubos_memory[m_current_swapchain_image_index]);
}

void Renderer::TriggerFramebufferResized()
{
	m_framebuffer_resized = true;
}

void Renderer::CreateGraphicsPipeline()
{	
	// Configure the viewport
	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(m_swapchain.GetExtent().width);
	viewport.height = static_cast<float>(m_swapchain.GetExtent().height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	// Configure the scissor rectangle
	VkRect2D scissor_rect = {};
	scissor_rect.offset = { 0, 0 };
	scissor_rect.extent = m_swapchain.GetExtent();

	// Pipeline layout
	VkPipelineLayoutCreateInfo pipeline_layout_info = {};
	pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_info.setLayoutCount = 1;
	pipeline_layout_info.pSetLayouts = &m_camera_data_descriptor_set_layout;

	if (vkCreatePipelineLayout(m_device.GetLogicalDeviceNative(), &pipeline_layout_info, nullptr, &m_pipeline_layout) != VK_SUCCESS)
		spdlog::error("Could not create a pipeline layout.");

	spdlog::info("Successfully created a pipeline layout.");

	// Structure used to configure the graphics pipeline
	auto* graphics_pipeline_info = new vk_wrapper::structs::VulkanGraphicsPipelineInfo();

	graphics_pipeline_info->cull_mode = vk_wrapper::enums::PolygonFaceCullMode::FrontFace;
	graphics_pipeline_info->discard_rasterizer_output = false;
	graphics_pipeline_info->enable_depth_bias = false;
	graphics_pipeline_info->line_width = 1.0f;
	graphics_pipeline_info->polygon_fill_mode = vk_wrapper::enums::PolygonFillMode::Fill;
	graphics_pipeline_info->scissor_rect = scissor_rect;
	graphics_pipeline_info->topology = vk_wrapper::enums::VertexTopologyType::TriangleList;
	graphics_pipeline_info->vertex_attribute_descs = Vertex::GetAttributeDescriptions();
	graphics_pipeline_info->vertex_binding_descs = Vertex::GetBindingDescriptions();
	graphics_pipeline_info->viewport = viewport;
	graphics_pipeline_info->winding_order = vk_wrapper::enums::TriangleWindingOrder::Clockwise;

	// Create the graphics pipeline
	m_graphics_pipeline.Create(
		m_device,
		graphics_pipeline_info,
		vk_wrapper::enums::PipelineType::Graphics,
		m_pipeline_layout,
		m_render_pass,
		{
			{ "./resources/shaders/basic.vert", vk_wrapper::enums::ShaderType::Vertex },
			{ "./resources/shaders/basic.frag", vk_wrapper::enums::ShaderType::Fragment }
		});

	// No need to keep the info around after pipeline creation
	delete graphics_pipeline_info;
}

void Renderer::CreateRenderPass()
{
	VkAttachmentDescription color_attachment = {};
	color_attachment.format = m_swapchain.GetFormat();
	color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference color_attachment_ref = {};
	color_attachment_ref.attachment = 0;
	color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &color_attachment_ref;

	VkSubpassDependency subpass_dependency = {};
	subpass_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	subpass_dependency.dstSubpass = 0;
	subpass_dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpass_dependency.srcAccessMask = 0;
	subpass_dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpass_dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo render_pass_info = {};
	render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	render_pass_info.attachmentCount = 1;
	render_pass_info.pAttachments = &color_attachment;
	render_pass_info.subpassCount = 1;
	render_pass_info.pSubpasses = &subpass;
	render_pass_info.dependencyCount = 1;
	render_pass_info.pDependencies = &subpass_dependency;

	if (vkCreateRenderPass(m_device.GetLogicalDeviceNative(), &render_pass_info, nullptr, &m_render_pass) != VK_SUCCESS)
	{
		spdlog::error("Could not create a render pass.");
		return;
	}

	spdlog::info("Successfully created a render pass.");
}

void Renderer::CreateFramebuffers()
{
	// Allocate enough memory to hold all framebuffers for the swapchain
	m_swapchain_framebuffers.resize(m_swapchain.GetImageViews().size());

	// Index for the for-loop
	std::uint32_t index = 0;

	// Create a new framebuffer for each image view
	for (const auto& image_view : m_swapchain.GetImageViews())
	{
		VkFramebufferCreateInfo framebuffer_info = {};
		framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebuffer_info.renderPass = m_render_pass;
		framebuffer_info.attachmentCount = 1;
		framebuffer_info.pAttachments = &image_view;
		framebuffer_info.width = m_swapchain.GetExtent().width;
		framebuffer_info.height = m_swapchain.GetExtent().height;
		framebuffer_info.layers = 1;

		if (vkCreateFramebuffer(m_device.GetLogicalDeviceNative(), &framebuffer_info, nullptr, &m_swapchain_framebuffers[index]) != VK_SUCCESS)
			spdlog::error("Could not create a framebuffer for the swapchain image view.");

		++index;
	}

	spdlog::info("Successfully created a framebuffer for each swapchain image view.");
}

void Renderer::CreateCommandPools()
{
	auto queue_family_indices = m_device.GetQueueFamilyIndices();

	VkCommandPoolCreateInfo graphics_pool_info = {};
	graphics_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	graphics_pool_info.queueFamilyIndex = queue_family_indices.graphics_family_index->first;

	if (vkCreateCommandPool(m_device.GetLogicalDeviceNative(), &graphics_pool_info, nullptr, &m_graphics_command_pool) != VK_SUCCESS)
	{
		spdlog::error("Could not create a graphics command pool.");
		return;
	}
	else
	{
		spdlog::info("Successfully created a graphics command pool.");
	}
}

void Renderer::CreateCommandBuffers()
{
	// We need one command buffer per swapchain framebuffer
	m_command_buffers.resize(m_swapchain_framebuffers.size());
	
	VkCommandBufferAllocateInfo graphics_command_buffer_allocate_info = {};
	graphics_command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	graphics_command_buffer_allocate_info.commandPool = m_graphics_command_pool;
	graphics_command_buffer_allocate_info.commandBufferCount = static_cast<std::uint32_t>(m_command_buffers.size());
	graphics_command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

	// Allocate the command buffers
	if (vkAllocateCommandBuffers(m_device.GetLogicalDeviceNative(), &graphics_command_buffer_allocate_info, m_command_buffers.data()) != VK_SUCCESS)
	{
		spdlog::error("Could not allocate command buffers.");
		return;
	}

	spdlog::info("Successfully allocated command buffers.");

	// Index tracking for the range-based for-loop
	std::uint32_t index = 0;

	// Record commands into command buffers
	for (const auto& command_buffer : m_command_buffers)
	{
		VkCommandBufferBeginInfo begin_info = {};
		begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
		
		// Begin recording
		if (vkBeginCommandBuffer(command_buffer, &begin_info) != VK_SUCCESS)
		{
			spdlog::error("Could not begin recording to command buffer #{}.", index);
			return;
		}

		// Black clear color
		VkClearValue clear_color = { 0.0f, 0.0f, 0.0f, 1.0f };

		// Prepare the render pass
		VkRenderPassBeginInfo render_pass_begin_info = {};
		render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		render_pass_begin_info.renderPass = m_render_pass;
		render_pass_begin_info.framebuffer = m_swapchain_framebuffers[index];
		render_pass_begin_info.renderArea.offset = { 0, 0 };
		render_pass_begin_info.renderArea.extent = m_swapchain.GetExtent();
		render_pass_begin_info.clearValueCount = 1;
		render_pass_begin_info.pClearValues = &clear_color;

		// Start the render pass
		vkCmdBeginRenderPass(m_command_buffers[index], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

		// Bind the graphics pipeline
		vkCmdBindPipeline(m_command_buffers[index], VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphics_pipeline.GetNative());

		// Bind the triangle vertex buffer
		VkBuffer vertex_buffers[] = { m_vertex_buffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(m_command_buffers[index], 0, 1, vertex_buffers, offsets);

		// Bind the camera UBO
		vkCmdBindDescriptorSets(
			m_command_buffers[index],
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			m_pipeline_layout,
			0,
			1,
			&m_descriptor_sets[index],
			0,
			nullptr);

		// Draw the triangle using hard-coded shader vertices
		vkCmdDraw(m_command_buffers[index], static_cast<std::uint32_t>(vertices.size()), 1, 0, 0);

		// End the render pass
		vkCmdEndRenderPass(m_command_buffers[index]);

		// Finish recording
		if (vkEndCommandBuffer(m_command_buffers[index]) != VK_SUCCESS)
		{
			spdlog::error("Could not record to command buffer #{}.", index);
			return;
		}

		++index;
	}
}

void Renderer::CreateSynchronizationObjects()
{
	m_in_flight_frame_image_available_semaphores.resize(global_settings::maximum_in_flight_frame_count);
	m_in_flight_render_finished_semaphores.resize(global_settings::maximum_in_flight_frame_count);
	m_in_flight_fences.resize(global_settings::maximum_in_flight_frame_count);

	VkSemaphoreCreateInfo semaphore_create_info = {};
	semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fence_create_info = {};
	fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;	// Fence is set to signaled on creation

	// Create all required semaphores
	for (auto index = 0; index < global_settings::maximum_in_flight_frame_count; ++index)
	{
		if (vkCreateSemaphore(m_device.GetLogicalDeviceNative(), &semaphore_create_info, nullptr, &m_in_flight_frame_image_available_semaphores[index]) != VK_SUCCESS ||
			vkCreateSemaphore(m_device.GetLogicalDeviceNative(), &semaphore_create_info, nullptr, &m_in_flight_render_finished_semaphores[index]) != VK_SUCCESS)
		{
			spdlog::error("Could not create one or both semaphores.");
			return;
		}
	}

	// Create all required fences
	for (auto index = 0; index < global_settings::maximum_in_flight_frame_count; ++index)
	{
		if (vkCreateFence(m_device.GetLogicalDeviceNative(), &fence_create_info, nullptr, &m_in_flight_fences[index]) != VK_SUCCESS)
		{
			spdlog::error("Could not create a fence.");
			return;
		}
	}
}

void Renderer::RecreateSwapchain(const Window& window)
{
	int width = 0;
	int height = 0;

	while (width == 0 || height == 0)
	{
		glfwGetFramebufferSize(m_window, &width, &height);
		glfwWaitEvents();
	}

	// Wait until the GPU finishes and clean-up the, now outdated, swapchain
	vkDeviceWaitIdle(m_device.GetLogicalDeviceNative());
	CleanUpSwapchain();

	// Create a new swapchain
	m_swapchain.Create(m_device, window);
	CreateRenderPass();
	CreateGraphicsPipeline();
	CreateFramebuffers();
	CreateUniformBuffers();
	CreateDescriptorPool();
	CreateDescriptorSets();
	CreateCommandBuffers();

	spdlog::info("Recreated the swapchain successfully.");
}

void Renderer::CleanUpSwapchain()
{
	for (auto index = 0; index < m_swapchain.GetImages().size(); ++index)
	{
		vkDestroyFramebuffer(m_device.GetLogicalDeviceNative(), m_swapchain_framebuffers[index], nullptr);
		vkDestroyBuffer(m_device.GetLogicalDeviceNative(), m_camera_ubos[index], nullptr);
		vkFreeMemory(m_device.GetLogicalDeviceNative(), m_camera_ubos_memory[index], nullptr);
	}

	vkDestroyDescriptorPool(m_device.GetLogicalDeviceNative(), m_descriptor_pool, nullptr);

	// No need to recreate the pool, freeing the command buffers is enough
	vkFreeCommandBuffers(m_device.GetLogicalDeviceNative(), m_graphics_command_pool, static_cast<std::uint32_t>(m_command_buffers.size()), m_command_buffers.data());

	m_graphics_pipeline.Destroy(m_device);
	vkDestroyPipelineLayout(m_device.GetLogicalDeviceNative(), m_pipeline_layout, nullptr);
	vkDestroyRenderPass(m_device.GetLogicalDeviceNative(), m_render_pass, nullptr);

	m_swapchain.Destroy(m_device);
}

void Renderer::CreateVertexBuffer()
{
	VkDeviceSize buffer_size = sizeof(Vertex) * vertices.size();

	VkBuffer staging_buffer;
	VkDeviceMemory staging_buffer_memory;

	// Create a staging buffer
	CreateBuffer(
		buffer_size,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		staging_buffer,
		staging_buffer_memory,
		m_device.GetLogicalDeviceNative(),
		m_device.GetPhysicalDeviceNative());

	void* data = nullptr;
	
	// Copy the data to a staging buffer
	vkMapMemory(m_device.GetLogicalDeviceNative(), staging_buffer_memory, 0, buffer_size, 0, &data);
	memcpy(data, vertices.data(), static_cast<size_t>(buffer_size));
	vkUnmapMemory(m_device.GetLogicalDeviceNative(), staging_buffer_memory);

	// Create a device local buffer to hold the vertex data in GPU memory
	CreateBuffer(
		buffer_size,
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		m_vertex_buffer,
		m_vertex_buffer_memory,
		m_device.GetLogicalDeviceNative(),
		m_device.GetPhysicalDeviceNative());

	// Copy the staging buffer to the GPU visible buffer
	CopyStagingBufferToDeviceLocalBuffer(
		m_device.GetLogicalDeviceNative(),
		staging_buffer,
		m_vertex_buffer,
		buffer_size,
		m_device.GetQueueNativeOfType(vk_wrapper::enums::VulkanQueueType::Graphics),
		m_graphics_command_pool);

	// Clean up the staging buffer since it is no longer needed
	vkDestroyBuffer(m_device.GetLogicalDeviceNative(), staging_buffer, nullptr);
	vkFreeMemory(m_device.GetLogicalDeviceNative(), staging_buffer_memory, nullptr);
}

void Renderer::CreateUniformBuffers()
{
	VkDeviceSize ubo_size = sizeof(CameraData);

	m_camera_ubos.resize(m_swapchain.GetImages().size());
	m_camera_ubos_memory.resize(m_swapchain.GetImages().size());

	// Create a camera data UBO for each image in the swapchain
	for (auto index = 0; index < m_swapchain.GetImages().size(); ++index)
	{
		CreateBuffer(
			ubo_size,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			m_camera_ubos[index],
			m_camera_ubos_memory[index],
			m_device.GetLogicalDeviceNative(),
			m_device.GetPhysicalDeviceNative());
	}
}

void Renderer::CreateDescriptorPool()
{
	VkDescriptorPoolSize descriptor_pool_size = {};
	descriptor_pool_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptor_pool_size.descriptorCount = static_cast<std::uint32_t>(m_swapchain.GetImages().size());

	VkDescriptorPoolCreateInfo pool_create_info = {};
	pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_create_info.poolSizeCount = 1;
	pool_create_info.pPoolSizes = &descriptor_pool_size;
	pool_create_info.maxSets = static_cast<std::uint32_t>(m_swapchain.GetImages().size());

	if (vkCreateDescriptorPool(m_device.GetLogicalDeviceNative(), &pool_create_info, nullptr, &m_descriptor_pool) != VK_SUCCESS)
	{
		spdlog::error("Could not create a descriptor pool.");
		return;
	}

	spdlog::info("Successfully created a descriptor pool.");
}

void Renderer::CreateDescriptorSetLayout()
{
	VkDescriptorSetLayoutBinding camera_data_layout_binding = {};
	camera_data_layout_binding.binding = 0;
	camera_data_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	camera_data_layout_binding.descriptorCount = 1;
	camera_data_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkDescriptorSetLayoutCreateInfo layout_create_info = {};
	layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layout_create_info.bindingCount = 1;
	layout_create_info.pBindings = &camera_data_layout_binding;

	if (vkCreateDescriptorSetLayout(m_device.GetLogicalDeviceNative(), &layout_create_info, nullptr, &m_camera_data_descriptor_set_layout) != VK_SUCCESS)
		spdlog::error("Could not create a descriptor set layout for the camera data.");
}

void Renderer::CreateDescriptorSets()
{
	std::vector<VkDescriptorSetLayout> layouts(m_swapchain.GetImages().size(), m_camera_data_descriptor_set_layout);

	VkDescriptorSetAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	alloc_info.descriptorPool = m_descriptor_pool;
	alloc_info.descriptorSetCount = static_cast<std::uint32_t>(m_swapchain.GetImages().size());
	alloc_info.pSetLayouts = layouts.data();

	m_descriptor_sets.resize(m_swapchain.GetImages().size());
	if (vkAllocateDescriptorSets(m_device.GetLogicalDeviceNative(), &alloc_info, m_descriptor_sets.data()) != VK_SUCCESS)
	{
		spdlog::error("Could not allocate descriptor sets.");
		return;
	}

	spdlog::info("Successfully allocated descriptor sets.");

	// Populate the newly allocated descriptor sets
	for (auto index = 0; index < m_swapchain.GetImages().size(); ++index)
	{
		VkDescriptorBufferInfo buffer_info = {};
		buffer_info.buffer = m_camera_ubos[index];
		buffer_info.offset = 0;
		buffer_info.range = sizeof(CameraData);

		VkWriteDescriptorSet descriptor_write = {};
		descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptor_write.dstSet = m_descriptor_sets[index];
		descriptor_write.dstBinding = 0;
		descriptor_write.dstArrayElement = 0;
		descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptor_write.descriptorCount = 1;
		descriptor_write.pBufferInfo = &buffer_info;

		vkUpdateDescriptorSets(m_device.GetLogicalDeviceNative(), 1, &descriptor_write, 0, nullptr);
	}
}

std::uint32_t Renderer::FindMemoryType(
	std::uint32_t type_filter,
	VkMemoryPropertyFlags properties,
	const VkPhysicalDevice& physical_device)
{
	VkPhysicalDeviceMemoryProperties memory_properties;
	vkGetPhysicalDeviceMemoryProperties(physical_device, &memory_properties);

	for (std::uint32_t index = 0; index < memory_properties.memoryTypeCount; ++index)
	{
		// Iterate through the types and check when the "type_filter" bit field is set to 1
		// Also check whether all required properties are supported
		if ((type_filter & (1 << index)) && (memory_properties.memoryTypes[index].propertyFlags & properties) == properties)
			return index;
	}

	spdlog::error("Could not find a suitable memory type");
	return 0;
}

void Renderer::CreateBuffer(
	VkDeviceSize size,
	VkBufferUsageFlags usage,
	VkMemoryPropertyFlags properties,
	VkBuffer& buffer,
	VkDeviceMemory& buffer_memory,
	const VkDevice& device,
	const VkPhysicalDevice physical_device)
{
	VkBufferCreateInfo buffer_info = {};
	buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_info.size = size;
	buffer_info.usage = usage;
	buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(device, &buffer_info, nullptr, &buffer) != VK_SUCCESS)
	{
		spdlog::error("Could not create a vertex buffer.");
		return;
	}

	VkMemoryRequirements memory_requirements;
	vkGetBufferMemoryRequirements(device, buffer, &memory_requirements);

	VkMemoryAllocateInfo allocation_info = {};
	allocation_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocation_info.allocationSize = memory_requirements.size;
	allocation_info.memoryTypeIndex = FindMemoryType(
		memory_requirements.memoryTypeBits,
		properties,
		physical_device);

	if (vkAllocateMemory(device, &allocation_info, nullptr, &buffer_memory) != VK_SUCCESS)
	{
		spdlog::error("Could not allocate memory for the vertex buffer.");
		return;
	}

	// Associate the memory with the buffer
	vkBindBufferMemory(device, buffer, buffer_memory, 0);
}

void Renderer::CopyStagingBufferToDeviceLocalBuffer(
	const VkDevice& device,
	const VkBuffer& source,
	const VkBuffer& destination,
	VkDeviceSize size,
	const VkQueue& transfer_queue,
	const VkCommandPool pool)
{
	// Not the best way to do this, needs refactoring
	// #TODO: REFACTOR THE COMMAND BUFFERS TO USE A SECOND GRAPHICS QUEUE FOR TRANSFERS
	VkCommandBufferAllocateInfo allocate_info = {};
	allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocate_info.commandBufferCount = 1;
	allocate_info.commandPool = pool;

	VkCommandBuffer cmd_buffer;
	vkAllocateCommandBuffers(device, &allocate_info, &cmd_buffer);

	VkCommandBufferBeginInfo begin_info = {};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	// Begin recording to the command buffer
	vkBeginCommandBuffer(cmd_buffer, &begin_info);

	VkBufferCopy copy_region = {};
	copy_region.size = size;

	// Issue a command that copies the staging buffer to the destination buffer
	vkCmdCopyBuffer(cmd_buffer, source, destination, 1, &copy_region);

	// End recording to the command buffer
	vkEndCommandBuffer(cmd_buffer);

	// Upload the staging buffer to the GPU by executing the transfer command buffer
	VkSubmitInfo submit_info = {};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &cmd_buffer;

	// Execute the commands
	vkQueueSubmit(transfer_queue, 1, &submit_info, VK_NULL_HANDLE);
	vkQueueWaitIdle(transfer_queue);

	vkFreeCommandBuffers(device, pool, 1, &cmd_buffer);
}
