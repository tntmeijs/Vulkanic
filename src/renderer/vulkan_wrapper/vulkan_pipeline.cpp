// Application
#include "core/viewport.hpp"
#include "miscellaneous/exceptions.hpp"
#include "vulkan_device.hpp"
#include "vulkan_pipeline.hpp"

using namespace vkc::core;
using namespace vkc::exception;
using namespace vkc::vk_wrapper;

// #TODO: Abstract pipeline layouts
// #TODO: Abstract render pipelines
void VulkanPipeline::Create(
	const VulkanDevice& device,
	const VulkanPipelineInfo* const pipeline_info,
	PipelineType type,
	VkPipelineLayout layout,
	VkRenderPass render_pass,
	const std::vector<std::pair<std::string, ShaderType>>& shader_files) noexcept(false)
{
	// Create a shader out of the specified shader source files
	VulkanShader shader;
	shader.Create(device, shader_files);

	// Create a pipeline based on the specified pipeline type
	switch (type)
	{
		case PipelineType::Graphics:
			CreateGraphicsPipeline(layout, render_pass, device, shader, pipeline_info);
			break;

		case PipelineType::Compute:
			CreateComputePipeline(pipeline_info);
			break;

		case PipelineType::RayTracing_NV:
			CreateRayTracingPipeline(pipeline_info);
			break;

		default:
			throw CriticalVulkanError("Invalid pipeline type specified.");
			break;
	}

	// No need to keep the shader around after the pipeline has been created
	shader.Destroy(device);
}

const VkPipeline& VulkanPipeline::GetNative() const noexcept(true)
{
	return m_pipeline;
}

void VulkanPipeline::Destroy(const VulkanDevice& device) const noexcept(true)
{
	vkDestroyPipeline(device.GetLogicalDeviceNative(), m_pipeline, nullptr);
}

void VulkanPipeline::CreateGraphicsPipeline(
	VkPipelineLayout layout,
	VkRenderPass render_pass,
	const VulkanDevice& device,
	const VulkanShader& shader,
	const VulkanPipelineInfo* const pipeline_info) noexcept(false)
{
	const VulkanGraphicsPipelineInfo *graphics_pipeline_info = dynamic_cast<const VulkanGraphicsPipelineInfo *>(pipeline_info);
	if (!graphics_pipeline_info)
	{
		throw CriticalVulkanError("Invalid pipeline info specified.");
	}

	VkPipelineVertexInputStateCreateInfo vertex_input_state = {};
	vertex_input_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertex_input_state.vertexBindingDescriptionCount = static_cast<std::uint32_t>(
		graphics_pipeline_info->vertex_binding_descs.size());
	vertex_input_state.pVertexBindingDescriptions = graphics_pipeline_info->vertex_binding_descs.data();
	vertex_input_state.vertexAttributeDescriptionCount = static_cast<std::uint32_t>(
		graphics_pipeline_info->vertex_attribute_descs.size());
	vertex_input_state.pVertexAttributeDescriptions = graphics_pipeline_info->vertex_attribute_descs.data();

	VkPipelineInputAssemblyStateCreateInfo input_assembly_state = {};
	input_assembly_state.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly_state.topology = static_cast<VkPrimitiveTopology>(
		graphics_pipeline_info->topology);
	input_assembly_state.primitiveRestartEnable = VK_FALSE;

	VkPipelineViewportStateCreateInfo viewport_state = {};
	viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport_state.viewportCount = 1;
	viewport_state.pViewports = &graphics_pipeline_info->viewport;
	viewport_state.scissorCount = 1;
	viewport_state.pScissors = &graphics_pipeline_info->scissor_rect;

	VkPipelineRasterizationStateCreateInfo rasterization_state = {};
	rasterization_state.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterization_state.depthClampEnable = graphics_pipeline_info->enable_depth_clamping;
	rasterization_state.rasterizerDiscardEnable = graphics_pipeline_info->discard_rasterizer_output;
	rasterization_state.polygonMode = static_cast<VkPolygonMode>(
		graphics_pipeline_info->polygon_fill_mode);
	rasterization_state.lineWidth = graphics_pipeline_info->line_width;
	rasterization_state.cullMode = static_cast<VkCullModeFlags>(
		graphics_pipeline_info->cull_mode);
	rasterization_state.frontFace = static_cast<VkFrontFace>(
		graphics_pipeline_info->winding_order);
	rasterization_state.depthBiasEnable = graphics_pipeline_info->enable_depth_bias;

	VkPipelineMultisampleStateCreateInfo multisample_state = {};
	multisample_state.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisample_state.sampleShadingEnable = VK_FALSE;
	multisample_state.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineColorBlendAttachmentState color_blend_attachment = {};
	color_blend_attachment.colorWriteMask =
		VK_COLOR_COMPONENT_R_BIT |
		VK_COLOR_COMPONENT_G_BIT |
		VK_COLOR_COMPONENT_B_BIT |
		VK_COLOR_COMPONENT_A_BIT;
	color_blend_attachment.blendEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo color_blend_state = {};
	color_blend_state.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	color_blend_state.logicOpEnable = VK_FALSE;
	color_blend_state.attachmentCount = 1;
	color_blend_state.pAttachments = &color_blend_attachment;

	VkGraphicsPipelineCreateInfo pipeline_create_info = {};
	pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipeline_create_info.stageCount = static_cast<std::uint32_t>(
		shader.GetPipelineShaderStageInfos().size());
	pipeline_create_info.pStages = shader.GetPipelineShaderStageInfos().data();
	pipeline_create_info.pVertexInputState = &vertex_input_state;
	pipeline_create_info.pInputAssemblyState = &input_assembly_state;
	pipeline_create_info.pViewportState = &viewport_state;
	pipeline_create_info.pRasterizationState = &rasterization_state;
	pipeline_create_info.pMultisampleState = &multisample_state;
	pipeline_create_info.pColorBlendState = &color_blend_state;
	pipeline_create_info.layout = layout;
	pipeline_create_info.renderPass = render_pass;
	pipeline_create_info.subpass = 0;

	auto result = vkCreateGraphicsPipelines(
		device.GetLogicalDeviceNative(),
		VK_NULL_HANDLE,
		1,
		&pipeline_create_info,
		nullptr,
		&m_pipeline);

	if (result != VK_SUCCESS)
	{
		throw CriticalVulkanError("Could not create a graphics pipeline.");
	}
}

void VulkanPipeline::CreateComputePipeline(
	const VulkanPipelineInfo* const pipeline_info) noexcept(false)
{
	const VulkanComputePipelineInfo* compute_pipeline_info = dynamic_cast<const VulkanComputePipelineInfo*>(pipeline_info);
	if (!compute_pipeline_info)
	{
		throw CriticalVulkanError("Invalid pipeline info specified.");
	}
}

void VulkanPipeline::CreateRayTracingPipeline(
	const VulkanPipelineInfo* const pipeline_info) noexcept(false)
{
	const VulkanRayTracingPipelineInfo* rt_pipeline_info = dynamic_cast<const VulkanRayTracingPipelineInfo*>(pipeline_info);
	if (!rt_pipeline_info)
	{
		throw CriticalVulkanError("Invalid pipeline info specified.");
	}
}
