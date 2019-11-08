#include "item_shader.h"
#include "glapi.h"
#include "xapi.h"
#include "wm.h"

int item_shader_load(ItemShader *shader) {
  if (!(shader->shader = shader_find(shaders, XInternAtom(display, "IG_SHADER_PIXMAP", False)))) {
    return 0;
  }
 
  shader->screen_attr = glGetUniformLocation(shader->shader->program, "screen");

  shader->picking_mode_attr = glGetUniformLocation(shader->shader->program, "picking_mode");
  shader->window_id_attr = glGetUniformLocation(shader->shader->program, "window_id");

  shader->window_sampler_attr = glGetUniformLocation(shader->shader->program, "window_sampler");
  shader->icon_sampler_attr = glGetUniformLocation(shader->shader->program, "icon_sampler");
  shader->icon_mask_sampler_attr = glGetUniformLocation(shader->shader->program, "icon_mask_sampler");
  shader->has_icon_attr = glGetUniformLocation(shader->shader->program, "has_icon");
  shader->has_icon_mask_attr = glGetUniformLocation(shader->shader->program, "has_icon_mask");
  
  return 1;
}
