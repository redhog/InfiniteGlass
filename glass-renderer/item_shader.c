#include "item_shader.h"
#include "glapi.h"
#include "xapi.h"
#include "wm.h"

int item_shader_load(ItemShader *shader) {
  if (!(shader->shader = shader_find(shaders, XInternAtom(display, "IG_SHADER_PIXMAP", False)))) {
    return 0;
  }
 
  shader->screen_attr = glGetUniformLocation(shader->shader->program, "screen");
  shader->size_attr = glGetUniformLocation(shader->shader->program, "size");
  
  shader->picking_mode_attr = glGetUniformLocation(shader->shader->program, "picking_mode");
  shader->window_id_attr = glGetUniformLocation(shader->shader->program, "window_id");

  shader->window_sampler_attr = glGetUniformLocation(shader->shader->program, "window_sampler");
  
  return 1;
}

int item_shader_initialized = 0;
ItemShader item_shader;

ItemShader *item_shader_get() {
  if (!item_shader_initialized) {
    if (!item_shader_load(&item_shader)) return NULL;
    item_shader_initialized =  1;
  }
  glUseProgram(item_shader.shader->program);
  return &item_shader;
}
