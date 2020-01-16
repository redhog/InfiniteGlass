#version 330 core
precision highp float;

uniform ivec2 size;

in vec2 px_coord;

out vec4 fragColor;

void main() {
  fragColor = vec4(1., 1., 1., 1.);
}
