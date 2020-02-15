uniform int IG_LAYER;

uniform int atom_IG_LAYER_MENU;
uniform int atom__NET_WM_WINDOW_TYPE_NORMAL;

uniform int _NET_WM_WINDOW_TYPE;
uniform int root__NET_ACTIVE_WINDOW;

vec4 get_border(int window_id, vec2 window_coord, vec2 window_size) {
  vec2 dcoord = window_coord;
  if (dcoord.x >= 0. && dcoord.x < window_size.x) dcoord.x = 0.;
  if (dcoord.y >= 0. && dcoord.y < window_size.y) dcoord.y = 0.;
  if (dcoord.x >= window_size.x) dcoord.x = dcoord.x - window_size.x;
  if (dcoord.y >= window_size.y) dcoord.y = dcoord.y - window_size.y;
  vec2 dist = ceil(abs(dcoord));

  if (IG_LAYER == atom_IG_LAYER_MENU || (_NET_WM_WINDOW_TYPE != 0 &&_NET_WM_WINDOW_TYPE != atom__NET_WM_WINDOW_TYPE_NORMAL)) {
    return vec4(0., 0., 0., 0.);
  } else if (root__NET_ACTIVE_WINDOW == window_id) {
    if ((dist.x == 3 && dist.y < 4) || (dist.y == 3 && dist.x < 4)) {
      return vec4(0., 0., 0., 1.);
    } else if ((dist.x == 4 && dist.y < 5) || (dist.y == 4 && dist.x < 5)) {
      return vec4(1., 1., 1., 1.);
    } else if (dist.x > 4 || dist.y > 4) {
      return vec4(0., 0., 0., 1.);
    } else {
      return vec4(0., 0., 0., 0.);
    }
  } else {
    if (dist.x >= 5 || dist.y >= 5) {
      return vec4(0., 0., 0., 1.);
    } else {
      return vec4(0., 0., 0., 0.);
    }
  }
}
