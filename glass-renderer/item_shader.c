#include "item_shader.h"

int item_shader_load(ItemShader *shader) {
  shader->screen_attr = glGetUniformLocation(shader->shader->program, "screen");

  shader->picking_mode_attr = glGetUniformLocation(shader->shader->program, "picking_mode");
  shader->window_id_attr = glGetUniformLocation(shader->shader->program, "window_id");
  return 1;
}
