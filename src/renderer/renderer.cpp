//////////////////////////////////////////////////////////////////////////

// Vulkanic
#include "miscellaneous/global_settings.hpp"
#include "renderer.hpp"
#include "vulkan_wrapper/vulkan_enums.hpp"
#include "vulkan_wrapper/vulkan_structures.hpp"
#include "vulkan_wrapper/vulkan_functions.hpp"
#include "miscellaneous/vulkanic_literals.hpp"

//////////////////////////////////////////////////////////////////////////

// GLM
#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

// STB
#include <miscellaneous/stb_defines.hpp>
#include <stb_image.h>

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
	glm::vec4 position;
	glm::vec4 color;
	glm::vec2 uv;

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
		std::vector<VkVertexInputAttributeDescription> attribs;

		VkVertexInputAttributeDescription position_attrib = {};
		position_attrib.binding = 0;
		position_attrib.location = 0;
		position_attrib.format = VK_FORMAT_R32G32B32_SFLOAT;
		position_attrib.offset = offsetof(Vertex, position);
		attribs.push_back(position_attrib);

		VkVertexInputAttributeDescription color_attrib = {};
		color_attrib.binding = 0;
		color_attrib.location = 1;
		color_attrib.format = VK_FORMAT_R32G32B32_SFLOAT;
		color_attrib.offset = offsetof(Vertex, color);
		attribs.push_back(color_attrib);

		VkVertexInputAttributeDescription uv_attrib = {};
		uv_attrib.binding = 0;
		uv_attrib.location = 2;
		uv_attrib.format = VK_FORMAT_R32G32_SFLOAT;
		uv_attrib.offset = offsetof(Vertex, uv);
		attribs.push_back(uv_attrib);

		return attribs;
	}
};

const std::vector<Vertex> vertices =
{
	// Position						// Color					// UV
	{ {  0.0f, -0.5f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f },	{ 0.5f, 0.0f } },
	{ { -0.5f,  0.5f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f },	{ 0.0f, 1.0f } },
	{ {  0.5f,  0.5f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f },	{ 1.0f, 1.0f } }
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
	, m_memory_manager(static_cast<std::uint32_t>(64_MB))
{}

Renderer::~Renderer()
{
	// Wait until the GPU finishes the current operation before cleaning-up resources
	vkDeviceWaitIdle(m_device.GetLogicalDeviceNative());

	CleanUpSwapchain();

	vkDestroySampler(m_device.GetLogicalDeviceNative(), m_texture_sampler, nullptr);
	vkDestroyImageView(m_device.GetLogicalDeviceNative(), m_texture_image_view, nullptr);
	vkDestroyImage(m_device.GetLogicalDeviceNative(), m_texture_image, nullptr);
	vkFreeMemory(m_device.GetLogicalDeviceNative(), m_texture_image_memory, nullptr);

	vkDestroyDescriptorSetLayout(m_device.GetLogicalDeviceNative(), m_camera_data_descriptor_set_layout, nullptr);

	m_vertex_buffer->Deallocate();

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

	// Clean-up all memory chunks
	m_memory_manager.Destroy();

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

	vk_wrapper::structs::VulkanRenderPassInfo render_pass_info = {};
	render_pass_info.attachment_descriptions = { color_attachment };
	render_pass_info.subpass_descriptions = { subpass };
	render_pass_info.subpass_dependencies = { subpass_dependency };

	m_render_pass.Create(m_device, render_pass_info);

	CreateDescriptorSetLayout();
	CreateGraphicsPipeline();
	CreateFramebuffers();
	CreateCommandPools();
	CreateTextureImage();
	CreateTextureImageView();
	CreateTextureSampler();
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
	rotate_amount += 0.00001f;

	CameraData cam_data = {};
	cam_data.model_matrix = glm::rotate(glm::mat4(1.0f), rotate_amount * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	cam_data.view_matrix = glm::lookAt(glm::vec3(0.0f, 0.25f, 0.75f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	cam_data.projection_matrix = glm::perspective(
		90.0f,
		static_cast<float>(m_swapchain.GetExtent().width) / static_cast<float>(m_swapchain.GetExtent().height),
		0.1f,
		1000.0f);

	m_camera_ubos[m_current_swapchain_image_index].Map(m_device.GetLogicalDeviceNative());
	memcpy(m_camera_ubos[m_current_swapchain_image_index].Data(), &cam_data, sizeof(cam_data));
	m_camera_ubos[m_current_swapchain_image_index].UnMap(m_device.GetLogicalDeviceNative());
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
		m_render_pass.GetNative(),
		{
			{ "./resources/shaders/basic.vert", vk_wrapper::enums::ShaderType::Vertex },
			{ "./resources/shaders/basic.frag", vk_wrapper::enums::ShaderType::Fragment }
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

void Renderer::CreateTextureImage()
{
	const char* path = "./resources/textures/uv_checker_map.png";
	int width, height, channel_count;
	auto* const pixel_data = stbi_load(
		path,
		&width,
		&height,
		&channel_count,
		STBI_rgb_alpha);

	VkDeviceSize image_size = width * height * STBI_rgb_alpha;

	if (!pixel_data)
	{
		spdlog::error("{} could not be loaded.", path);
		return;
	}

	auto staging_buffer = m_memory_manager.AllocateBuffer(m_device.GetLogicalDeviceNative(), m_device.GetPhysicalDeviceNative(), image_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	// Copy image data to the staging buffer
	staging_buffer.Map(m_device.GetLogicalDeviceNative());
	auto buffer_data_ptr = staging_buffer.Data();
	memcpy(buffer_data_ptr, pixel_data, static_cast<size_t>(image_size));
	staging_buffer.UnMap(m_device.GetLogicalDeviceNative());

	// Image data has been saved, no need to keep it around anymore
	stbi_image_free(pixel_data);

	// Create a Vulkan image
	vk_wrapper::func::CreateImage(
		m_device.GetLogicalDeviceNative(),
		width,
		height,
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		m_texture_image);

	// Allocate memory for the image
	VkMemoryRequirements memory_requirements;
	vkGetImageMemoryRequirements(
		m_device.GetLogicalDeviceNative(),
		m_texture_image,
		&memory_requirements);

	VkMemoryAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.allocationSize = memory_requirements.size;
	alloc_info.memoryTypeIndex = vk_wrapper::func::FindMemoryTypeIndex(
		memory_requirements.memoryTypeBits,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		m_device.GetPhysicalDeviceNative());

	if (vkAllocateMemory(
		m_device.GetLogicalDeviceNative(),
		&alloc_info,
		nullptr,
		&m_texture_image_memory) != VK_SUCCESS)
	{
		spdlog::error("Could not allocate image memory.");
		return;
	}

	vkBindImageMemory(
		m_device.GetLogicalDeviceNative(),
		m_texture_image,
		m_texture_image_memory,
		0);

	// Transition the image so it can be used as a transfer destination
	TransitionImageLayout(
		m_device,
		m_graphics_command_pool,
		m_device.GetQueueNativeOfType(vk_wrapper::enums::VulkanQueueType::Graphics),
		m_texture_image,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	// Perform the buffer to image data transfer
	CopyBufferToImage(
		m_device,
		m_graphics_command_pool,
		m_device.GetQueueNativeOfType(vk_wrapper::enums::VulkanQueueType::Graphics),
		staging_buffer.Buffer(),
		m_texture_image,
		static_cast<std::uint32_t>(width),
		static_cast<std::uint32_t>(height));

	// Transition the image so it can be used to read from in a shader
	TransitionImageLayout(
		m_device,
		m_graphics_command_pool,
		m_device.GetQueueNativeOfType(vk_wrapper::enums::VulkanQueueType::Graphics),
		m_texture_image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	// Delete the staging buffer
	staging_buffer.Deallocate();
}

void Renderer::CreateTextureImageView()
{
	VkImageViewCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	info.image = m_texture_image;
	info.viewType = VK_IMAGE_VIEW_TYPE_2D;
	info.format = VK_FORMAT_R8G8B8A8_UNORM;
	info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	info.subresourceRange.layerCount = 1;
	info.subresourceRange.levelCount = 1;
	info.subresourceRange.baseArrayLayer = 0;
	info.subresourceRange.baseMipLevel = 0;
	info.components = {
		VK_COMPONENT_SWIZZLE_IDENTITY,
		VK_COMPONENT_SWIZZLE_IDENTITY,
		VK_COMPONENT_SWIZZLE_IDENTITY,
		VK_COMPONENT_SWIZZLE_IDENTITY
	};

	if (vkCreateImageView(m_device.GetLogicalDeviceNative(), &info, nullptr, &m_texture_image_view) != VK_SUCCESS)
	{
		spdlog::error("Could not create an image view.");
		return;
	}
}

void Renderer::CreateTextureSampler()
{
	VkSamplerCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	info.minFilter = VK_FILTER_LINEAR;
	info.magFilter = VK_FILTER_LINEAR;
	info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	info.anisotropyEnable = VK_TRUE;
	info.maxAnisotropy = 16;
	info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	info.unnormalizedCoordinates = VK_FALSE;
	info.compareEnable = VK_FALSE;
	info.compareOp = VK_COMPARE_OP_ALWAYS;
	info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	info.mipLodBias = 0.0f;
	info.minLod = 0.0f;
	info.maxLod = 0.0f;

	if (vkCreateSampler(m_device.GetLogicalDeviceNative(), &info, nullptr, &m_texture_sampler) != VK_SUCCESS)
	{
		spdlog::error("Could not create a texture sampler.");
		return;
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
		render_pass_begin_info.renderPass = m_render_pass.GetNative();
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
		VkBuffer vertex_buffers[] = { m_vertex_buffer->Buffer() };
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

	vk_wrapper::structs::VulkanRenderPassInfo render_pass_info = {};
	render_pass_info.attachment_descriptions = { color_attachment };
	render_pass_info.subpass_descriptions = { subpass };
	render_pass_info.subpass_dependencies = { subpass_dependency };

	m_render_pass.Create(m_device, render_pass_info);

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
		m_camera_ubos[index].Deallocate();
	}

	vkDestroyDescriptorPool(m_device.GetLogicalDeviceNative(), m_descriptor_pool, nullptr);

	// No need to recreate the pool, freeing the command buffers is enough
	vkFreeCommandBuffers(m_device.GetLogicalDeviceNative(), m_graphics_command_pool, static_cast<std::uint32_t>(m_command_buffers.size()), m_command_buffers.data());

	m_graphics_pipeline.Destroy(m_device);
	vkDestroyPipelineLayout(m_device.GetLogicalDeviceNative(), m_pipeline_layout, nullptr);
	
	m_render_pass.Destroy(m_device);
	m_swapchain.Destroy(m_device);
}

void Renderer::CreateVertexBuffer()
{
	VkDeviceSize buffer_size = sizeof(Vertex) * vertices.size();

	// Create a staging buffer (CPU-visible)
	auto staging_buffer = m_memory_manager.AllocateBuffer(
		m_device.GetLogicalDeviceNative(),
		m_device.GetPhysicalDeviceNative(),
		buffer_size,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	
	// Copy the data to a staging buffer
	staging_buffer.Map(m_device.GetLogicalDeviceNative());
	memcpy(staging_buffer.Data(), vertices.data(), static_cast<size_t>(buffer_size));
	staging_buffer.UnMap(m_device.GetLogicalDeviceNative());

	// Create the GPU-visible vertex buffer
	m_vertex_buffer = std::make_unique<memory::VirtualBuffer>(m_memory_manager.AllocateBuffer(
		m_device.GetLogicalDeviceNative(),
		m_device.GetPhysicalDeviceNative(),
		buffer_size,
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));

	// Copy the staging buffer to the GPU visible buffer
	CopyStagingBufferToDeviceLocalBuffer(
		m_device,
		&staging_buffer,
		m_vertex_buffer.get(),
		m_device.GetQueueNativeOfType(vk_wrapper::enums::VulkanQueueType::Graphics),
		m_graphics_command_pool);

	// Clean up the staging buffer since it is no longer needed
	staging_buffer.Deallocate();
}

void Renderer::CreateUniformBuffers()
{
	VkDeviceSize ubo_size = sizeof(CameraData);

	m_camera_ubos.reserve(m_swapchain.GetImages().size());

	// Create a camera data UBO for each image in the swapchain
	for (auto index = 0; index < m_swapchain.GetImages().size(); ++index)
	{
		m_camera_ubos.push_back(m_memory_manager.AllocateBuffer(
			m_device.GetLogicalDeviceNative(),
			m_device.GetPhysicalDeviceNative(),
			ubo_size,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));
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
		buffer_info.buffer = m_camera_ubos[index].Buffer();
		buffer_info.offset = 0;
		buffer_info.range = sizeof(CameraData);

		VkDescriptorImageInfo image_info = {};
		image_info.sampler = m_texture_sampler;
		image_info.imageView = m_texture_image_view;
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
	const memory::VirtualBuffer* const  source,
	const memory::VirtualBuffer* const destination,
	const VkQueue& queue,
	const VkCommandPool pool)
{
	auto cmd_buffer = BeginSingleTimeCommands(pool, device);

	VkBufferCopy copy_region = {};
	copy_region.srcOffset = source->Offset();
	copy_region.size = source->Size();
	copy_region.dstOffset = destination->Offset();

	// Issue a command that copies the staging buffer to the destination buffer
	vkCmdCopyBuffer(cmd_buffer, source->Buffer(), destination->Buffer(), 1, &copy_region);

	EndSingleTimeCommands(device, pool, cmd_buffer, queue);
}

VkCommandBuffer vkc::Renderer::BeginSingleTimeCommands(const VkCommandPool& pool, const vk_wrapper::VulkanDevice& device)
{
	VkCommandBufferAllocateInfo allocate_info = {};
	allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocate_info.commandBufferCount = 1;
	allocate_info.commandPool = pool;

	VkCommandBuffer cmd_buffer;
	vkAllocateCommandBuffers(device.GetLogicalDeviceNative(), &allocate_info, &cmd_buffer);

	VkCommandBufferBeginInfo begin_info = {};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	// Begin recording to the command buffer
	vkBeginCommandBuffer(cmd_buffer, &begin_info);

	return cmd_buffer;
}

void vkc::Renderer::EndSingleTimeCommands(
	const vk_wrapper::VulkanDevice& device,
	const VkCommandPool& pool,
	const VkCommandBuffer& cmd_buffer,
	const VkQueue& queue)
{
	// End recording to the command buffer
	vkEndCommandBuffer(cmd_buffer);

	// Upload the staging buffer to the GPU by executing the transfer command buffer
	VkSubmitInfo submit_info = {};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &cmd_buffer;

	// Execute the commands
	vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE);
	vkQueueWaitIdle(queue);

	vkFreeCommandBuffers(device.GetLogicalDeviceNative(), pool, 1, &cmd_buffer);
}

void Renderer::TransitionImageLayout(
	const vk_wrapper::VulkanDevice& device,
	const VkCommandPool& pool,
	const VkQueue& queue,
	const VkImage& image,
	const VkImageLayout& current_layout,
	const VkImageLayout& new_layout)
{
	auto cmd_buffer = BeginSingleTimeCommands(pool, device);

	VkPipelineStageFlags source_stage;
	VkPipelineStageFlags destination_stage;

	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = current_layout;
	barrier.newLayout = new_layout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.baseMipLevel = 0;

	if (current_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (current_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else
	{
		spdlog::error("Unsupported image layout transition.");
		return;
	}

	vkCmdPipelineBarrier(
		cmd_buffer,
		source_stage,
		destination_stage,
		0,			// Flags
		0,			// No memory barriers
		nullptr,
		0,			// No buffer memory barriers
		nullptr,
		1,			// One image memory barrier
		&barrier);

	EndSingleTimeCommands(device, pool, cmd_buffer, queue);
}

void Renderer::CopyBufferToImage(
	const vk_wrapper::VulkanDevice& device,
	const VkCommandPool& pool,
	const VkQueue& queue,
	const VkBuffer& buffer,
	const VkImage& image,
	std::uint32_t width,
	std::uint32_t height)
{
	auto cmd_buffer = BeginSingleTimeCommands(pool, device);

	VkBufferImageCopy region = {};
	
	// No padding
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;

	// Use no mip / array levels
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.layerCount = 1;

	// Copy the entire image
	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = { width, height, 1 };

	// Queue the copy operation
	vkCmdCopyBufferToImage(cmd_buffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	EndSingleTimeCommands(device, pool, cmd_buffer, queue);
}
