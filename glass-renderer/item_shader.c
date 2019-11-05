#include "item_shader.h"

int item_shader_load(ItemShader *shader) {
  shader->width_attr = glGetUniformLocation(shader->shader->program, "width");
  shader->height_attr = glGetUniformLocation(shader->shader->program, "height");
  shader->screen_attr = glGetUniformLocation(shader->shader->program, "screen");

  shader->picking_mode_attr = glGetUniformLocation(shader->shader->program, "picking_mode");
  shader->window_id_attr = glGetUniformLocation(shader->shader->program, "window_id");
  return 1;
}
