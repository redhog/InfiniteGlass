#version 330 core
precision highp float;

uniform ivec2 size;
uniform float root_IG_WORLD_ZOOM;

in vec2 px_coord;

out vec4 fragColor;

#define PI 3.14159265

void main() {
  float radius = root_IG_WORLD_ZOOM * float(min(size.x, size.y)) / 2.;
  vec2 distvec = vec2(px_coord) - vec2(size) / 2.;
  float dist = sqrt(pow(distvec.x, 2) + pow(distvec.y, 2));
  if (dist < radius) {
    float w = PI/2. - asin(dist / radius);
    float l = w / (PI/2.);
    //fragColor = vec4(gl_FragCoord.z, 0., 1. - gl_FragCoord.z, .5);
    fragColor = vec4(l, l, l, 1.);
    //fragColor = vec4(px_coord.x/size.x, px_coord.y/size.y, 1.-px_coord.x/size.x, 1.); //vec4(1., 1., 1., 1.); 
  } else {
    fragColor = vec4(0., 0., 0., 1.);
  }
}
