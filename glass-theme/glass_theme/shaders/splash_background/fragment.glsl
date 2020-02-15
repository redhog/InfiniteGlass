#version 330 core
precision highp float;

uniform ivec2 size;
uniform float root_IG_WORLD_ZOOM;
uniform float root_IG_WORLD_ALPHA;

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
    fragColor = vec4(l, l, l, root_IG_WORLD_ALPHA);
  } else {
    fragColor = vec4(0., 0., 0., root_IG_WORLD_ALPHA);
  }
}
