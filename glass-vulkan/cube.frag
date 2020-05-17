#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

//layout (early_fragment_tests) in;

layout (binding = 1) uniform sampler2D tex[1];

layout (location = 0) in vec4 texcoord;
layout (location = 1) in vec3 frag_pos;
layout (location = 0) out vec4 uFragColor;


void main() {
	int tind = int(frag_pos.z);
	uFragColor = texture(tex[tind], texcoord.xy);
	uFragColor.a = 1.0;
//	uFragColor = vec4(0.0, 0.0, abs(frag_pos.x), 1.0);
//	uFragColor = vec4(0.0, 0.0, 0.0, 0.0);
//	uFragColor.r = frag_pos.y;
//	uFragColor.b = frag_pos.x;
}
