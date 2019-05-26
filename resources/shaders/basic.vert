#version 460

layout(binding=0) uniform CameraData
{
    mat4 model;
    mat4 view;
    mat4 projection;
} cam_data;

layout(location=0) in vec4 a_position;
layout(location=1) in vec4 a_color;
layout(location=2) in vec2 a_uv;

layout(location=0) out vec4 v_color;
layout(location=1) out vec2 v_uv;

void main()
{
	gl_Position = cam_data.projection * cam_data.view * cam_data.model * a_position;
    v_color = a_color;
    v_uv = a_uv;
}
