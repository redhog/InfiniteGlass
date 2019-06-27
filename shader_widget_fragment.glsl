#version 330 core
precision highp float;

in vec2 window_coord;
in float geometry_size;

uniform int picking_mode;
uniform float window_id;

uniform sampler2D texture_sampler;
uniform vec4 transform;

out vec4 fragColor;

vec4 texture_coord;

void main() {
  mat4 transform_mat = transpose(mat4(
    1./transform[2], 0., 0., -transform[0]/transform[2],
    0., 1./transform[3], 0., -transform[1]/transform[3],
    0., 0., 1., 0.,
    0., 0., 0., 1.
  ));
  texture_coord = transform_mat * vec4(window_coord, 0, 1.);

  if (picking_mode == 1) {
    fragColor = vec4(texture_coord.x, texture_coord.y, window_id, 1.);
  } else {
    fragColor = texture(texture_sampler, texture_coord.xy).rgba;
  }
}
