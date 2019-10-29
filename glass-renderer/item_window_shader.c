#include "item_window_shader.h"
#include "glapi.h"
#include "xapi.h"

int item_window_shader_load(ItemWindowShader *shader) {
  shader->base.base.name = XInternAtom(display, "SHADER_PIXMAP", False);
 
  if (!item_shader_load((ItemShader *) shader)) {
    return 0;
  }
    
  shader->window_sampler_attr = glGetUniformLocation(shader->base.base.program, "window_sampler");
  shader->icon_sampler_attr = glGetUniformLocation(shader->base.base.program, "icon_sampler");
  shader->icon_mask_sampler_attr = glGetUniformLocation(shader->base.base.program, "icon_mask_sampler");
  shader->has_icon_attr = glGetUniformLocation(shader->base.base.program, "has_icon");
  shader->has_icon_mask_attr = glGetUniformLocation(shader->base.base.program, "has_icon_mask");
  return 1;
}

int item_window_shader_initialized = 0;
ItemWindowShader item_window_shader;

ItemWindowShader *item_window_shader_get() {
  if (!item_window_shader_initialized) {
    if (!item_window_shader_load(&item_window_shader)) return NULL;
    item_window_shader_initialized =  1;
  }
  glUseProgram(item_window_shader.base.base.program);
  return &item_window_shader;
}
