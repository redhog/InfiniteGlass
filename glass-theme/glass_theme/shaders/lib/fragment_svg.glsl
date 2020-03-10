uniform sampler2D IG_CONTENT;
uniform vec4 IG_CONTENT_transform;

vec4 get_svg_content_any(vec2 scaled_window_coord, sampler2D content, vec4 transform) {
  mat4 transform_mat = transpose(mat4(
    1./transform[2], 0., 0., -transform[0]/transform[2],
    0., 1./transform[3], 0., -transform[1]/transform[3],
    0., 0., 1., 0.,
    0., 0., 0., 1.
  ));
  vec4 texture_coord = transform_mat * vec4(scaled_window_coord, 0, 1.);
  return texture(content, texture_coord.xy).rgba;
}

vec4 get_svg_content(vec2 scaled_window_coord) {
  return get_svg_content_any(scaled_window_coord, IG_CONTENT, IG_CONTENT_transform);
}
