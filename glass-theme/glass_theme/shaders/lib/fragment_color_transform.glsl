vec4 transform_color(vec4 color, int transform_id, int default_transform_id) {
  if (transform_id == 0) transform_id = default_transform_id;
  if (transform_id == 1) {
    return COLOR_TRANSFORM_1 * color;
  } else if (transform_id == 2) { 
    return COLOR_TRANSFORM_2 * color;
  } else if (transform_id == 3) { 
    return COLOR_TRANSFORM_3 * color;
  } else {
    return color;
  }
}
