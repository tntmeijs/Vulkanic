// Glslang (needs to be included before Vulkan header)
#include <SPIRV/GlslangToSpv.h>
#include <StandAlone/DirStackFileIncluder.h>

// Application
#include "miscellaneous/exceptions.hpp"
#include "vulkan_device.hpp"
#include "vulkan_shader.hpp"

// Spdlog
#include <spdlog/spdlog.h>

// C++ standard
#include <fstream>

using namespace glslang;
using namespace vkc::vk_wrapper;
using namespace vkc::vk_wrapper::enums;

void vkc::vk_wrapper::VulkanShader::Create(
	const VulkanDevice& device,
	const std::vector<std::pair<std::string, ShaderType>>& shader_files) noexcept(false)
{
	// Create all pipeline shader stage create infos
	for (const auto& shader : shader_files)
	{
		const auto shader_bytecode = GetSPIRV(shader.first);
		const auto shader_module = CreateShaderModule(device, shader_bytecode);

		VkPipelineShaderStageCreateInfo shader_stage_info = {};
		shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shader_stage_info.stage = static_cast<VkShaderStageFlagBits>(shader.second);
		shader_stage_info.module = shader_module;
		shader_stage_info.pName = "main";

		m_shader_modules.push_back(shader_module);
		m_shader_stage_infos.push_back(shader_stage_info);
	}
}

void VulkanShader::Destroy(const VulkanDevice& device) const noexcept(true)
{
	for (const auto& shader_module : m_shader_modules)
	{
		vkDestroyShaderModule(
			device.GetLogicalDeviceNative(),
			shader_module,
			nullptr);
	}
}

const std::vector<VkPipelineShaderStageCreateInfo>& VulkanShader::GetPipelineShaderStageInfos() const noexcept(true)
{
	return m_shader_stage_infos;
}

std::vector<std::uint32_t> VulkanShader::GetSPIRV(
	const std::string& path) const noexcept(false)
{
	if (!glsl_lang_initialized)
	{
		InitializeProcess();
		glsl_lang_initialized = true;
	}

	// Open the GLSL file
	std::ifstream shader_file(path);

	if (!shader_file.is_open())
	{
		// Could not open the file
		throw exception::CriticalIOError("Could not open shader file.");
	}

	// Dump the contents of the entire file into a string
	std::string glsl_str(
		(std::istreambuf_iterator<char>(shader_file)),
		std::istreambuf_iterator<char>());

	// Convert to a C-string
	const auto glsl_cstr = glsl_str.c_str();

	// Determine shader type
	const auto shader_stage_type = GetShaderStageType(path);

	// Configure a Glslang shader object
	const int client_input_semantics_version = 100;	// #define VULKAN 100
	const auto vulkan_client_version = EShTargetVulkan_1_0;
	const auto target_version = EShTargetSpv_1_0;

	TShader shader(shader_stage_type);
	shader.setStrings(&glsl_cstr, 1);
	shader.setEnvInput(
		EShSourceGlsl,
		shader_stage_type,
		EShClientVulkan,
		client_input_semantics_version);
	shader.setEnvClient(EShClientVulkan, vulkan_client_version);
	shader.setEnvTarget(EShTargetSpv, target_version);

	// Values copied from the default configuration file generated by
	// glslangvalidator.exe
	TBuiltInResource default_built_in_resource							= {};
	default_built_in_resource.maxLights									= 32;
	default_built_in_resource.maxClipPlanes								= 6;
	default_built_in_resource.maxTextureUnits							= 32;
	default_built_in_resource.maxTextureCoords							= 32;
	default_built_in_resource.maxVertexAttribs							= 64;
	default_built_in_resource.maxVertexUniformComponents				= 4096;
	default_built_in_resource.maxVaryingFloats							= 64;
	default_built_in_resource.maxVertexTextureImageUnits				= 32;
	default_built_in_resource.maxCombinedTextureImageUnits				= 80;
	default_built_in_resource.maxTextureImageUnits						= 32;
	default_built_in_resource.maxFragmentUniformComponents				= 4096;
	default_built_in_resource.maxDrawBuffers							= 32;
	default_built_in_resource.maxVertexUniformVectors					= 128;
	default_built_in_resource.maxVaryingVectors							= 8;
	default_built_in_resource.maxFragmentUniformVectors					= 16;
	default_built_in_resource.maxVertexOutputVectors					= 16;
	default_built_in_resource.maxFragmentInputVectors					= 15;
	default_built_in_resource.minProgramTexelOffset						= -8;
	default_built_in_resource.maxProgramTexelOffset						= 7;
	default_built_in_resource.maxClipDistances							= 8;
	default_built_in_resource.maxComputeWorkGroupCountX					= 65535;
	default_built_in_resource.maxComputeWorkGroupCountY					= 65535;
	default_built_in_resource.maxComputeWorkGroupCountZ					= 65535;
	default_built_in_resource.maxComputeWorkGroupSizeX					= 1024;
	default_built_in_resource.maxComputeWorkGroupSizeY					= 1024;
	default_built_in_resource.maxComputeWorkGroupSizeZ					= 64;
	default_built_in_resource.maxComputeUniformComponents				= 1024;
	default_built_in_resource.maxComputeTextureImageUnits				= 16;
	default_built_in_resource.maxComputeImageUniforms					= 8;
	default_built_in_resource.maxComputeAtomicCounters					= 8;
	default_built_in_resource.maxComputeAtomicCounterBuffers			= 1;
	default_built_in_resource.maxVaryingComponents						= 60;
	default_built_in_resource.maxVertexOutputComponents					= 64;
	default_built_in_resource.maxGeometryInputComponents				= 64;
	default_built_in_resource.maxGeometryOutputComponents				= 128;
	default_built_in_resource.maxFragmentInputComponents				= 128;
	default_built_in_resource.maxImageUnits								= 8;
	default_built_in_resource.maxCombinedImageUnitsAndFragmentOutputs	= 8;
	default_built_in_resource.maxCombinedShaderOutputResources			= 8;
	default_built_in_resource.maxImageSamples							= 0;
	default_built_in_resource.maxVertexImageUniforms					= 0;
	default_built_in_resource.maxTessControlImageUniforms				= 0;
	default_built_in_resource.maxTessEvaluationImageUniforms			= 0;
	default_built_in_resource.maxGeometryImageUniforms					= 0;
	default_built_in_resource.maxFragmentImageUniforms					= 8;
	default_built_in_resource.maxCombinedImageUniforms					= 8;
	default_built_in_resource.maxGeometryTextureImageUnits				= 16;
	default_built_in_resource.maxGeometryOutputVertices					= 256;
	default_built_in_resource.maxGeometryTotalOutputComponents			= 1024;
	default_built_in_resource.maxGeometryUniformComponents				= 1024;
	default_built_in_resource.maxGeometryVaryingComponents				= 64;
	default_built_in_resource.maxTessControlInputComponents				= 128;
	default_built_in_resource.maxTessControlOutputComponents			= 128;
	default_built_in_resource.maxTessControlTextureImageUnits			= 16;
	default_built_in_resource.maxTessControlUniformComponents			= 1024;
	default_built_in_resource.maxTessControlTotalOutputComponents		= 4096;
	default_built_in_resource.maxTessEvaluationInputComponents			= 128;
	default_built_in_resource.maxTessEvaluationOutputComponents			= 128;
	default_built_in_resource.maxTessEvaluationTextureImageUnits		= 16;
	default_built_in_resource.maxTessEvaluationUniformComponents		= 1024;
	default_built_in_resource.maxTessPatchComponents					= 120;
	default_built_in_resource.maxPatchVertices							= 32;
	default_built_in_resource.maxTessGenLevel							= 64;
	default_built_in_resource.maxViewports								= 16;
	default_built_in_resource.maxVertexAtomicCounters					= 0;
	default_built_in_resource.maxTessControlAtomicCounters				= 0;
	default_built_in_resource.maxTessEvaluationAtomicCounters			= 0;
	default_built_in_resource.maxGeometryAtomicCounters					= 0;
	default_built_in_resource.maxFragmentAtomicCounters					= 8;
	default_built_in_resource.maxCombinedAtomicCounters					= 8;
	default_built_in_resource.maxAtomicCounterBindings					= 1;
	default_built_in_resource.maxVertexAtomicCounterBuffers				= 0;
	default_built_in_resource.maxTessControlAtomicCounterBuffers		= 0;
	default_built_in_resource.maxTessEvaluationAtomicCounterBuffers		= 0;
	default_built_in_resource.maxGeometryAtomicCounterBuffers			= 0;
	default_built_in_resource.maxFragmentAtomicCounterBuffers			= 1;
	default_built_in_resource.maxCombinedAtomicCounterBuffers			= 1;
	default_built_in_resource.maxAtomicCounterBufferSize				= 16384;
	default_built_in_resource.maxTransformFeedbackBuffers				= 4;
	default_built_in_resource.maxTransformFeedbackInterleavedComponents	= 64;
	default_built_in_resource.maxCullDistances							= 8;
	default_built_in_resource.maxCombinedClipAndCullDistances			= 8;
	default_built_in_resource.maxSamples								= 4;
	default_built_in_resource.maxMeshOutputVerticesNV					= 256;
	default_built_in_resource.maxMeshOutputPrimitivesNV					= 512;
	default_built_in_resource.maxMeshWorkGroupSizeX_NV					= 32;
	default_built_in_resource.maxMeshWorkGroupSizeY_NV					= 1;
	default_built_in_resource.maxMeshWorkGroupSizeZ_NV					= 1;
	default_built_in_resource.maxTaskWorkGroupSizeX_NV					= 32;
	default_built_in_resource.maxTaskWorkGroupSizeY_NV					= 1;
	default_built_in_resource.maxTaskWorkGroupSizeZ_NV					= 1;
	default_built_in_resource.maxMeshViewCountNV						= 4;

	EShMessages messages = static_cast<EShMessages>(EShMsgSpvRules | EShMsgVulkanRules);
	const std::uint32_t default_version = 100;

	// Preprocessing GLSL
	DirStackFileIncluder shader_includer = {};
	shader_includer.pushExternalLocalDirectory(path);

	std::string preprocessed_glsl_str = {};

	if (!shader.preprocess(
		&default_built_in_resource,
		default_version,
		ENoProfile,
		false,
		false,
		messages,
		&preprocessed_glsl_str,
		shader_includer))
	{
		spdlog::error("Could not pre-process GLSL for: \"{}\".", path);
		spdlog::error(shader.getInfoLog());
		spdlog::error(shader.getInfoDebugLog());

		throw exception::CriticalVulkanError("Failed to preprocess GLSL.");
	}

	// Save preprocessed GLSL to the shader (replaces old GLSL)
	const auto preprocessed_glsl_cstr = preprocessed_glsl_str.c_str();
	shader.setStrings(&preprocessed_glsl_cstr, 1);

	// Parse the shader
	if (!shader.parse(
		&default_built_in_resource,
		default_version,
		false,
		messages))
	{
		spdlog::error("Could not parse GLSL for: \"{}\".", path);
		spdlog::error(shader.getInfoLog());
		spdlog::error(shader.getInfoDebugLog());

		throw exception::CriticalVulkanError("Failed to parse GLSL.");
	}

	// Link the shader to a program
	TProgram shader_program = {};
	shader_program.addShader(&shader);

	if (!shader_program.link(messages))
	{
		spdlog::error("Could not link GLSL program for: \"{}\".", path);
		spdlog::error(shader.getInfoLog());
		spdlog::error(shader.getInfoDebugLog());

		throw exception::CriticalVulkanError("Failed to link GLSL program.");
	}

	std::vector<std::uint32_t> spirv = {};
	spv::SpvBuildLogger spirv_logger = {};
	SpvOptions spirv_options = {};
	GlslangToSpv(
		*shader_program.getIntermediate(shader_stage_type),
		spirv,
		&spirv_logger,
		&spirv_options);

	return spirv;
}

VkShaderModule VulkanShader::CreateShaderModule(
	const VulkanDevice& device,
	const std::vector<std::uint32_t>& bytecode) const noexcept(false)
{
	VkShaderModule shader_module = {};
	VkShaderModuleCreateInfo module_create_info = {};

	module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	module_create_info.codeSize = sizeof(std::uint32_t) * bytecode.size();
	module_create_info.pCode = bytecode.data();

	auto result = vkCreateShaderModule(
		device.GetLogicalDeviceNative(),
		&module_create_info,
		nullptr,
		&shader_module);

	if (result != VK_SUCCESS)
	{
		throw exception::CriticalVulkanError("Could not create shader module.");
	}

	return shader_module;
}

EShLanguage VulkanShader::GetShaderStageType(
	const std::string& path) const noexcept(false)
{
	// Get the file extension
	auto extension_start = path.rfind('.');
	auto extension = path.substr(extension_start + 1);

	// Find the correct shader type
	if (extension == "vert")
	{
		return EShLangVertex;
	}
	else if (extension == "tesc")
	{
		return EShLangTessControl;
	}
	else if (extension == "tese")
	{
		return EShLangTessEvaluation;
	}
	else if (extension == "geom")
	{
		return EShLangGeometry;
	}
	else if (extension == "frag")
	{
		return EShLangFragment;
	}
	else if (extension == "comp")
	{
		return EShLangCompute;
	}
	else
	{
		throw exception::CriticalIOError("Unknown shader file extension.");
		return EShLangCount;
	}
}
