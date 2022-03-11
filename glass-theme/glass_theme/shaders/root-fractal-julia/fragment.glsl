#version 330 core
precision highp float;

/* This code is available as a stand-alone fractal demo
   at https://www.shadertoy.com/view/sdXfzl
*/

#include "resource://glass_theme/shaders/lib/fragment_color_transform.glsl"

uniform ivec2 size;
uniform vec4 root_IG_VIEW_DESKTOP_VIEW;
uniform int IG_COLOR_TRANSFORM;

in vec2 px_coord;

out vec4 fragColor;

vec2 iResolution = vec2(float(size.x), float(size.y));

const int iter = 30;
const float s = 7.;
const vec2 c = vec2(0.35, 0.55);

float log_b(float x, float b) {
  return log(x) / log(b);
}

void main() {
  vec3 col;
  vec2 coord = px_coord;
  coord.y = iResolution.y - coord.y;

  vec2 z = (coord / iResolution.xy) * root_IG_VIEW_DESKTOP_VIEW.zw + root_IG_VIEW_DESKTOP_VIEW.xy;
  float currentScale = root_IG_VIEW_DESKTOP_VIEW.z;

  int i;
  int local_iter = iter;
  if (currentScale < 1.) {
    local_iter += int(log_b(1. / currentScale, 1.01) * 0.06);
  }
  for(i=0; i < local_iter; i++) {
      float x = (z.x * z.x - z.y * z.y) + c.x;
      float y = (z.y * z.x + z.x * z.y) + c.y;

      if((x * x + y * y) > 4.0) break;
      z.x = x;
      z.y = y;
  }

  float v = pow(float(i), s) / pow(float(local_iter), s);
  v = 2. * abs(v - 0.5);
  
  fragColor = vec4(v, v, v, 1.);
  fragColor = transform_color(fragColor, IG_COLOR_TRANSFORM, IG_COLOR_TRANSFORM_DEFAULT);
}
