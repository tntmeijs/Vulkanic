#version 460

layout(location=0) out vec4 vertex_color;

vec4 positions[3] = vec4[]
(
    vec4( 0.0, -0.5, 0.0, 1.0),
	vec4(-0.5,  0.5, 0.0, 1.0),
    vec4( 0.5,  0.5, 0.0, 1.0)
);

vec4 colors[3] = vec4[]
(
    vec4(1.0, 0.0, 0.0, 1.0),
    vec4(0.0, 1.0, 0.0, 1.0),
    vec4(0.0, 0.0, 1.0, 1.0)
);

void main()
{
	gl_Position = positions[gl_VertexIndex];
	vertex_color = colors[gl_VertexIndex];
}
