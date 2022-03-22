#version 150 core
precision highp float;

uniform ivec2 size;
uniform float root_IG_WORLD_ALPHA;

in vec2 px_coord;

out vec4 fragColor;

void main() {
  fragColor = vec4(0., 0., 0., root_IG_WORLD_ALPHA);
}
