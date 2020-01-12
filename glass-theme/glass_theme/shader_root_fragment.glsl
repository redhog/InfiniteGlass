#version 330 core
precision highp float;

#define ICON_CUTOFF_1 .4
#define ICON_CUTOFF_2 .3

uniform ivec2 size;

in vec2 px_coord;

out vec4 fragColor;

void main() {
  fragColor = vec4(px_coord.x/size.x, px_coord.y/size.y, 1.-px_coord.x/size.x, 1.);
}
