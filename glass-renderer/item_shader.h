#ifndef ITEM_SHADER
#define ITEM_SHADER

#include "shader.h"

typedef struct {
  Shader *shader;

  GLint screen_attr;
  GLint size_attr;
 
  GLint picking_mode_attr;
  GLint window_id_attr;

  GLint window_sampler_attr;
} ItemShader;

extern int item_shader_load(ItemShader *shader);
ItemShader *item_shader_get();

#endif
