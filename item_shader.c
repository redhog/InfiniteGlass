#include "item_shader.h"

int item_shader_load(ItemShader *shader) {
  if (!shader_load((Shader *) shader)) {
    return 0;
  }

  shader.screen_attr = glGetUniformLocation(shader.base.program, "screen");
  shader.coords_attr = glGetAttribLocation(shader.base.program, "coords");

  shader.picking_mode_attr = glGetUniformLocation(shader.base.program, "picking_mode");
  shader.window_id_attr = glGetUniformLocation(shader.base.program, "window_id");
}
