uniform sampler2D IG_CONTENT;
uniform vec4 IG_CONTENT_transform;

vec4 get_svg_content(vec2 scaled_window_coord) {
  mat4 transform_mat = transpose(mat4(
    1./IG_CONTENT_transform[2], 0., 0., -IG_CONTENT_transform[0]/IG_CONTENT_transform[2],
    0., 1./IG_CONTENT_transform[3], 0., -IG_CONTENT_transform[1]/IG_CONTENT_transform[3],
    0., 0., 1., 0.,
    0., 0., 0., 1.
  ));
  vec4 texture_coord = transform_mat * vec4(scaled_window_coord, 0, 1.);
  return texture(IG_CONTENT, texture_coord.xy).rgba;
}
