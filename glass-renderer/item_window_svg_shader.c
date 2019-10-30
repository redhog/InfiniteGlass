#include "item_window_svg_shader.h"
#include "glapi.h"
#include "xapi.h"
#include "wm.h"

int item_window_svg_shader_load(ItemWindowSvgShader *shader) {
  if (!(shader->base.shader = shader_find(shaders, XInternAtom(display, "IG_SHADER_SVG", False)))) {
    return 0;
  }
 
  if (!item_shader_load((ItemShader *) shader)) {
    return 0;
  }
    
  shader->texture_attr = glGetUniformLocation(shader->base.shader->program, "texture_sampler");
  shader->transform_attr = glGetUniformLocation(shader->base.shader->program, "transform");
  return 1;
}

int item_window_svg_shader_initialized = 0;
ItemWindowSvgShader item_window_svg_shader;

ItemWindowSvgShader *item_window_svg_shader_get() {
  if (!item_window_svg_shader_initialized) {
    if (!item_window_svg_shader_load(&item_window_svg_shader)) return NULL;
    item_window_svg_shader_initialized =  1;
  }
  glUseProgram(item_window_svg_shader.base.shader->program);
  return &item_window_svg_shader;
}
