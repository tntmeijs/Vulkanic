//////////////////////////////////////////////////////////////////////////

// Vulkanic
#include "miscellaneous/global_settings.hpp"
#include "renderer.hpp"
#include "renderer/vertex.hpp"
#include "vulkan_wrapper/vulkan_functions.hpp"
#include "miscellaneous/vulkanic_literals.hpp"

//////////////////////////////////////////////////////////////////////////

// GLM
#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

// STB
#include <stb_image.h>

// VulkanMemoryManager
#include <vk_mem_alloc.h>

//////////////////////////////////////////////////////////////////////////

// C++ standard
#include <algorithm>
#include <fstream>
#include <set>
#include <string>
#include <vector>

//////////////////////////////////////////////////////////////////////////

using namespace vkc;

// Hard-coded model
const std::vector<VertexPCT> vertices =
{
	// Position					// Color				// UV
	{ {  0.0f, -0.5f, 0.0f },	{ 1.0f, 1.0f, 1.0f, },	{ 0.5f, 0.0f } },
	{ { -0.5f,  0.5f, 0.0f },	{ 1.0f, 1.0f, 1.0f, },	{ 0.0f, 1.0f } },
	{ {  0.5f,  0.5f, 0.0f },	{ 1.0f, 1.0f, 1.0f, },	{ 1.0f, 1.0f } }
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
{}

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

	// Initialize the memory manager
	memory::MemoryManager::GetInstance().Initialize(m_device);

	// Create the swapchain (also creates all related objects such as image views)
	m_swapchain.Create(m_device, window);

	// Create a render pass
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

	vk_wrapper::VulkanRenderPassInfo render_pass_info = {};
	render_pass_info.attachment_descriptions = { color_attachment };
	render_pass_info.subpass_descriptions = { subpass };
	render_pass_info.subpass_dependencies = { subpass_dependency };

	m_render_pass.Create(m_device, render_pass_info);

	CreateDescriptorSetLayout();
	CreateGraphicsPipeline();
	CreateFramebuffers();

	m_graphics_command_pool.Create(m_device, vk_wrapper::CommandPoolType::Graphics);

	CreateVertexBuffer();
	CreateUniformBuffers();

	m_uv_map_checker_texture.Create("./resources/textures/uv_checker_map.png", VK_FORMAT_R8G8B8A8_UNORM, m_device, m_graphics_command_pool);
	m_default_sampler.Create(m_device);

	CreateDescriptorPool();
	CreateDescriptorSets();
	RecordFrameCommands();
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
	submit_info.pCommandBuffers = &m_graphics_command_buffers.GetNative(m_current_swapchain_image_index);
	submit_info.signalSemaphoreCount = sizeof(signal_semaphores) / sizeof(signal_semaphores[0]);
	submit_info.pSignalSemaphores = signal_semaphores;

	// Fence completed, reset its state
	vkResetFences(m_device.GetLogicalDeviceNative(), 1, &m_in_flight_fences[m_frame_index]);

	// Submit the command queue
	if (vkQueueSubmit(m_device.GetQueueNativeOfType(vk_wrapper::VulkanQueueType::Graphics), 1, &submit_info, m_in_flight_fences[m_frame_index]) != VK_SUCCESS)
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
	result = vkQueuePresentKHR(m_device.GetQueueNativeOfType(vk_wrapper::VulkanQueueType::Present), &present_info);

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
	rotate_amount += 0.00001f;

	CameraData cam_data = {};
	cam_data.model_matrix = glm::rotate(glm::mat4(1.0f), rotate_amount * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	cam_data.view_matrix = glm::lookAt(glm::vec3(0.0f, 0.25f, 0.75f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	cam_data.projection_matrix = glm::perspective(
		90.0f,
		static_cast<float>(m_swapchain.GetExtent().width) / static_cast<float>(m_swapchain.GetExtent().height),
		0.1f,
		1000.0f);

	m_camera_ubos[m_current_swapchain_image_index].Update(cam_data);
}

void Renderer::TriggerFramebufferResized()
{
	m_framebuffer_resized = true;
}

void vkc::Renderer::Destroy()
{
	// Wait until the GPU finishes the current operation before cleaning-up resources
	vkDeviceWaitIdle(m_device.GetLogicalDeviceNative());

	CleanUpSwapchain();

	m_default_sampler.Destroy(m_device);
	m_uv_map_checker_texture.Destroy(m_device);

	vkDestroyDescriptorSetLayout(m_device.GetLogicalDeviceNative(), m_camera_data_descriptor_set_layout, nullptr);

	// This will automatically clean up any allocated buffers and images
	memory::MemoryManager::GetInstance().Destroy();

	for (auto index = 0; index < global_settings::maximum_in_flight_frame_count; ++index)
	{
		vkDestroySemaphore(m_device.GetLogicalDeviceNative(), m_in_flight_frame_image_available_semaphores[index], nullptr);
		vkDestroySemaphore(m_device.GetLogicalDeviceNative(), m_in_flight_render_finished_semaphores[index], nullptr);
	}

	for (auto index = 0; index < global_settings::maximum_in_flight_frame_count; ++index)
	{
		vkDestroyFence(m_device.GetLogicalDeviceNative(), m_in_flight_fences[index], nullptr);
	}

	m_graphics_command_pool.Destroy(m_device);

	m_device.Destroy();

#ifdef _DEBUG
	m_debug_messenger.Destroy(m_instance);
#endif

	m_swapchain.DestroySurface(m_instance);
	m_instance.Destroy();
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
	auto* graphics_pipeline_info = new vk_wrapper::VulkanGraphicsPipelineInfo();

	graphics_pipeline_info->cull_mode = vk_wrapper::PolygonFaceCullMode::FrontFace;
	graphics_pipeline_info->discard_rasterizer_output = false;
	graphics_pipeline_info->enable_depth_bias = false;
	graphics_pipeline_info->line_width = 1.0f;
	graphics_pipeline_info->polygon_fill_mode = vk_wrapper::PolygonFillMode::Fill;
	graphics_pipeline_info->scissor_rect = scissor_rect;
	graphics_pipeline_info->topology = vk_wrapper::VertexTopologyType::TriangleList;
	graphics_pipeline_info->vertex_attribute_descs = VertexPCT::GetAttributeDescriptions();
	graphics_pipeline_info->vertex_binding_descs = VertexPCT::GetBindingDescriptions();
	graphics_pipeline_info->viewport = viewport;
	graphics_pipeline_info->winding_order = vk_wrapper::TriangleWindingOrder::Clockwise;

	// Create the graphics pipeline
	m_graphics_pipeline.Create(
		m_device,
		graphics_pipeline_info,
		vk_wrapper::PipelineType::Graphics,
		m_pipeline_layout,
		m_render_pass.GetNative(),
		{
			{ "./resources/shaders/basic.vert", vk_wrapper::ShaderType::Vertex },
			{ "./resources/shaders/basic.frag", vk_wrapper::ShaderType::Fragment }
		});

	// No need to keep the info around after pipeline creation
	delete graphics_pipeline_info;
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
		framebuffer_info.renderPass = m_render_pass.GetNative();
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

void Renderer::RecordFrameCommands()
{
	m_graphics_command_buffers.Create(
		m_device,
		m_graphics_command_pool,
		static_cast<std::uint32_t>(m_swapchain_framebuffers.size()));

	// Record commands into command buffers
	for (auto i = 0; i < m_swapchain_framebuffers.size(); ++i)
	{
		// Begin recording
		m_graphics_command_buffers.BeginRecording(i, vk_wrapper::CommandBufferUsage::SimultaneousUse);

		// Black clear color
		VkClearValue clear_color = { 0.0f, 0.0f, 0.0f, 1.0f };

		// Prepare the render pass
		VkRenderPassBeginInfo render_pass_begin_info = {};
		render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		render_pass_begin_info.renderPass = m_render_pass.GetNative();
		render_pass_begin_info.framebuffer = m_swapchain_framebuffers[i];
		render_pass_begin_info.renderArea.offset = { 0, 0 };
		render_pass_begin_info.renderArea.extent = m_swapchain.GetExtent();
		render_pass_begin_info.clearValueCount = 1;
		render_pass_begin_info.pClearValues = &clear_color;

		// Start the render pass
		vkCmdBeginRenderPass(m_graphics_command_buffers.GetNative(i), &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

		// Bind the graphics pipeline
		vkCmdBindPipeline(m_graphics_command_buffers.GetNative(i), VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphics_pipeline.GetNative());

		// Bind the triangle vertex buffer
		VkBuffer vertex_buffers[] = { m_vertex_buffer.buffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(m_graphics_command_buffers.GetNative(i), 0, 1, vertex_buffers, offsets);

		// Bind the camera UBO
		vkCmdBindDescriptorSets(
			m_graphics_command_buffers.GetNative(i),
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			m_pipeline_layout,
			0,
			1,
			&m_descriptor_sets[i],
			0,
			nullptr);

		// Draw the triangle using hard-coded shader vertices
		vkCmdDraw(m_graphics_command_buffers.GetNative(i), static_cast<std::uint32_t>(vertices.size()), 1, 0, 0);

		// End the render pass
		vkCmdEndRenderPass(m_graphics_command_buffers.GetNative(i));

		// Finish recording
		m_graphics_command_buffers.StopRecording(i);
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

	// Create a new render pass
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

	vk_wrapper::VulkanRenderPassInfo render_pass_info = {};
	render_pass_info.attachment_descriptions = { color_attachment };
	render_pass_info.subpass_descriptions = { subpass };
	render_pass_info.subpass_dependencies = { subpass_dependency };

	m_render_pass.Create(m_device, render_pass_info);

	CreateGraphicsPipeline();
	CreateFramebuffers();
	CreateUniformBuffers();
	CreateDescriptorPool();
	CreateDescriptorSets();
	RecordFrameCommands();

	spdlog::info("Recreated the swapchain successfully.");
}

void Renderer::CleanUpSwapchain()
{
	for (auto index = 0; index < m_swapchain.GetImages().size(); ++index)
	{
		vkDestroyFramebuffer(m_device.GetLogicalDeviceNative(), m_swapchain_framebuffers[index], nullptr);
		m_camera_ubos[index].Destroy();
	}

	vkDestroyDescriptorPool(m_device.GetLogicalDeviceNative(), m_descriptor_pool, nullptr);

	// No need to recreate the pool, freeing the command buffers is enough
	m_graphics_command_buffers.Destroy(m_device, m_graphics_command_pool);

	m_graphics_pipeline.Destroy(m_device);
	vkDestroyPipelineLayout(m_device.GetLogicalDeviceNative(), m_pipeline_layout, nullptr);
	
	m_render_pass.Destroy(m_device);
	m_swapchain.Destroy(m_device);
}

void Renderer::CreateVertexBuffer()
{
	VkDeviceSize buffer_size = sizeof(VertexPCT) * vertices.size();

	// Create a CPU-visible staging buffer
	memory::BufferAllocationInfo staging_buffer_alloc_info = {};
	staging_buffer_alloc_info.buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	staging_buffer_alloc_info.buffer_create_info.size = buffer_size;
	staging_buffer_alloc_info.buffer_create_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	staging_buffer_alloc_info.buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	staging_buffer_alloc_info.allocation_info.usage = VMA_MEMORY_USAGE_CPU_ONLY;

	auto staging_buffer = memory::MemoryManager::GetInstance().Allocate(staging_buffer_alloc_info);

	// Copy the data to a staging buffer
	auto data = memory::MemoryManager::GetInstance().MapBuffer(staging_buffer);
	memcpy(data, vertices.data(), static_cast<size_t>(buffer_size));
	memory::MemoryManager::GetInstance().UnMapBuffer(staging_buffer);

	// Create a GPU-visible vertex buffer
	memory::BufferAllocationInfo vertex_buffer_alloc_info = {};
	vertex_buffer_alloc_info.buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	vertex_buffer_alloc_info.buffer_create_info.size = buffer_size;
	vertex_buffer_alloc_info.buffer_create_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	vertex_buffer_alloc_info.buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	vertex_buffer_alloc_info.allocation_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;

	// Not deleting the texture here causes the vertex memory to zero-out (right before vertex buffer memory allocation)
	//m_test_texture.Destroy(m_device);

	m_vertex_buffer = memory::MemoryManager::GetInstance().Allocate(vertex_buffer_alloc_info);

	// Copy the staging buffer to the GPU visible buffer
	CopyStagingBufferToDeviceLocalBuffer(
		m_device,
		staging_buffer,
		m_vertex_buffer,
		m_device.GetQueueNativeOfType(vk_wrapper::VulkanQueueType::Graphics),
		m_graphics_command_pool);

	// Clean up the staging buffer since it is no longer needed
	memory::MemoryManager::GetInstance().Free(staging_buffer);
}

void Renderer::CreateUniformBuffers()
{
	m_camera_ubos.reserve(m_swapchain.GetImages().size());

	// Create a camera data UBO for each image in the swapchain
	for (auto index = 0; index < m_swapchain.GetImages().size(); ++index)
	{
		vk_wrapper::VulkanUniformBuffer ubo = {};
		ubo.Create<CameraData>();
		m_camera_ubos.push_back(ubo);
	}
}

void Renderer::CreateDescriptorPool()
{
	VkDescriptorPoolSize descriptor_pool_sizes[2] = {};

	descriptor_pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptor_pool_sizes[0].descriptorCount = static_cast<std::uint32_t>(m_swapchain.GetImages().size());

	descriptor_pool_sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptor_pool_sizes[1].descriptorCount = static_cast<std::uint32_t>(m_swapchain.GetImages().size());

	VkDescriptorPoolCreateInfo pool_create_info = {};
	pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_create_info.poolSizeCount = sizeof(descriptor_pool_sizes) / sizeof(VkDescriptorPoolSize);
	pool_create_info.pPoolSizes = descriptor_pool_sizes;
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

	VkDescriptorSetLayoutBinding sampler_layout_binding = {};
	sampler_layout_binding.binding = 1;
	sampler_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	sampler_layout_binding.descriptorCount = 1;
	sampler_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding layouts[] = { camera_data_layout_binding, sampler_layout_binding };

	VkDescriptorSetLayoutCreateInfo layout_create_info = {};
	layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layout_create_info.bindingCount = sizeof(layouts) / sizeof(VkDescriptorSetLayoutBinding);
	layout_create_info.pBindings = layouts;

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
		buffer_info.buffer = m_camera_ubos[index].GetNative();
		buffer_info.offset = 0;
		buffer_info.range = sizeof(CameraData);

		VkDescriptorImageInfo image_info = {};
		image_info.sampler = m_default_sampler.GetNative();
		image_info.imageView = m_uv_map_checker_texture.GetImageView();
		image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		VkWriteDescriptorSet descriptor_writes[2] = {};

		descriptor_writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptor_writes[0].dstSet = m_descriptor_sets[index];
		descriptor_writes[0].dstBinding = 0;
		descriptor_writes[0].dstArrayElement = 0;
		descriptor_writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptor_writes[0].descriptorCount = 1;
		descriptor_writes[0].pBufferInfo = &buffer_info;

		descriptor_writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptor_writes[1].dstSet = m_descriptor_sets[index];
		descriptor_writes[1].dstBinding = 1;
		descriptor_writes[1].dstArrayElement = 0;
		descriptor_writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptor_writes[1].descriptorCount = 1;
		descriptor_writes[1].pImageInfo = &image_info;

		vkUpdateDescriptorSets(
			m_device.GetLogicalDeviceNative(),
			sizeof(descriptor_writes) / sizeof(VkWriteDescriptorSet),
			descriptor_writes,
			0,
			nullptr);
	}
}

void Renderer::CopyStagingBufferToDeviceLocalBuffer(
	const vk_wrapper::VulkanDevice& device,
	const memory::VulkanBuffer& source,
	const memory::VulkanBuffer& destination,
	const VkQueue& queue,
	const vk_wrapper::VulkanCommandPool& pool)
{
	vk_wrapper::VulkanCommandBuffer cmd_buffer;
	cmd_buffer.Create(device, pool, 1);
	cmd_buffer.BeginRecording(vk_wrapper::CommandBufferUsage::OneTimeSubmit);

	VkBufferCopy copy_region = {};
	copy_region.srcOffset = source.info.offset;
	copy_region.size = source.info.size;
	copy_region.dstOffset = destination.info.offset;

	// Issue a command that copies the staging buffer to the destination buffer
	vkCmdCopyBuffer(cmd_buffer.GetNative(), source.buffer, destination.buffer, 1, &copy_region);
	
	// Execute the commands
	cmd_buffer.StopRecording();
	cmd_buffer.Submit(queue);
	vkQueueWaitIdle(queue);

	// No longer need the command buffer to stick around
	cmd_buffer.Destroy(device, pool);
}
