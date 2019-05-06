#version 460

layout(binding=0) uniform CameraData
{
    mat4 model;
    mat4 view;
    mat4 projection;
} cam_data;

layout(location=0) in vec3 a_position;
layout(location=1) in vec3 a_color;

layout(location=0) out vec4 output_color;

void main()
{
	gl_Position = cam_data.projection * cam_data.view * cam_data.model * vec4(a_position, 1.0);
    output_color = vec4(a_color, 1.0);
}
