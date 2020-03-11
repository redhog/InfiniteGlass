uniform int picking_mode;
uniform int window_id;
uniform int widget_id;

vec4 get_picking(int window_id, vec2 scaled_window_coord) {
  if (scaled_window_coord.x < 0. || scaled_window_coord.x > 1. || scaled_window_coord.y < 0. || scaled_window_coord.y > 1.) {
    return vec4(0.,0.,-1.,-1.);
  } else {
    // window_id is 32-3=29 bits.
    // fraction part of float32 is 23 bits. 2^23 = 8388608
    // widget_id is 23*2 - 29 = 17 bits
    // see glass-renderer/view.c:view_pick for the decoding of this
    return vec4(
      scaled_window_coord.x,
      scaled_window_coord.y,
      (widget_id * 64) + int(floor(window_id / 8388608)),
      window_id - 8388608 * int(floor(window_id / 8388608)));
  }
}
