#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
layout(std140, binding = 0) uniform buf {
        mat4 MVP;
        vec4 position[12*3];
        vec4 attr[12*3];
} ubuf;

layout (location = 0) in vec4 inPosition;

layout (location = 0) out vec4 texcoord;
layout (location = 1) out vec3 frag_pos;

void main() {
	gl_Position = ubuf.MVP * vec4(inPosition.xyz, 1.0);
//	gl_Position = vec4(inPosition.xyz, 1.0);
//	gl_Position = ubuf.MVP * ubuf.position[gl_VertexIndex];
	texcoord = ubuf.attr[gl_VertexIndex % 6];
//	frag_pos = gl_Position.xyz;
	frag_pos = vec4(ubuf.MVP * vec4(inPosition.xyz, 1.0)).xyz;
	frag_pos.z = inPosition.w;
}
