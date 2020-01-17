#version 330 core
precision highp float;

uniform ivec2 size;

in vec2 px_coord;

out vec4 fragColor;

void main() {

  // fragColor = vec4(px_coord.x/size.x, px_coord.y/size.y, 1.-px_coord.x/size.x, 1.);
  // fragColor = vec4(gl_FragCoord.z, 0., 1. - gl_FragCoord.z, 1.);

  fragColor = vec4(0., 0., 0., 1.);
}
