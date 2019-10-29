#include "item_window_svg_shader.h"
#include "glapi.h"
#include "xapi.h"

int item_window_svg_shader_load(ItemWindowSvgShader *shader) {
  shader->base.base.name = XInternAtom(display, "SHADER_SVG", False);
 
  if (!item_shader_load((ItemShader *) shader)) {
    return 0;
  }
    
  shader->texture_attr = glGetUniformLocation(shader->base.base.program, "texture_sampler");
  shader->transform_attr = glGetUniformLocation(shader->base.base.program, "transform");
  return 1;
}

int item_window_svg_shader_initialized = 0;
ItemWindowSvgShader item_window_svg_shader;

ItemWindowSvgShader *item_window_svg_shader_get() {
  if (!item_window_svg_shader_initialized) {
    if (!item_window_svg_shader_load(&item_window_svg_shader)) return NULL;
    item_window_svg_shader_initialized =  1;
  }
  glUseProgram(item_window_svg_shader.base.base.program);
  return &item_window_svg_shader;
}
