uniform sampler2D window_sampler;

vec4 get_pixmap(vec2 scaled_window_coord) {
  return texture(window_sampler, scaled_window_coord).rgba;
}
