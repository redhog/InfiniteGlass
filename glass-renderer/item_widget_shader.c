#include "item_widget_shader.h"
#include "glapi.h"


int item_widget_shader_load(ItemWidgetShader *shader) {
  if (!shader->base.base.vertex_src_file) shader->base.base.vertex_src_file = "shader_window_vertex.glsl";
  if (!shader->base.base.geometry_src_file) shader->base.base.geometry_src_file = "shader_window_geometry.glsl";
  if (!shader->base.base.fragment_src_file) shader->base.base.fragment_src_file = "shader_widget_fragment.glsl";
 
  if (!item_shader_load((ItemShader *) shader)) {
    return 0;
  }
    
  shader->texture_attr = glGetUniformLocation(shader->base.base.program, "texture_sampler");
  shader->transform_attr = glGetUniformLocation(shader->base.base.program, "transform");
  return 1;
}

int item_widget_shader_initialized = 0;
ItemWidgetShader item_widget_shader;

ItemWidgetShader *item_widget_shader_get() {
  if (!item_widget_shader_initialized) {
    if (!item_widget_shader_load(&item_widget_shader)) return NULL;
    item_widget_shader_initialized =  1;
  }
  glUseProgram(item_widget_shader.base.base.program);
  return &item_widget_shader;
}
