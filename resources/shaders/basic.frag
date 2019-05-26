#version 460

layout(binding=1) uniform sampler2D texture_sampler;

layout(location=0) in vec4 v_color;
layout(location=1) in vec2 v_uv;

layout(location=0) out vec4 output_color;

void main()
{
	output_color = texture(texture_sampler, v_uv) * v_color;
}
