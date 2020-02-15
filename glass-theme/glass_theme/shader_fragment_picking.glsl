vec4 get_picking(int window_id, vec2 scaled_window_coord) {
  if (scaled_window_coord.x < 0. || scaled_window_coord.x > 1. || scaled_window_coord.y < 0. || scaled_window_coord.y > 1.) {
    return vec4(0.,0.,-1.,-1.);
  } else {
    return vec4(scaled_window_coord.x, scaled_window_coord.y, window_id / 65536, window_id - 65536 * (window_id / 65536));
  }
}
