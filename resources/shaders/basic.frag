#version 460

layout(location=0) in vec4 vertex_color;

layout(location=0) out vec4 output_color;

void main()
{
	output_color = vertex_color;
}
