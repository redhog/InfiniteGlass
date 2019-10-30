#ifndef ITEM_SHADER
#define ITEM_SHADER

#include "shader.h"

typedef struct {
  Shader *shader;

  GLint width_attr;
  GLint height_attr;
 
  GLint screen_attr;
  GLint coords_attr;
 
  GLint picking_mode_attr;
  GLint window_id_attr;
} ItemShader;

extern int item_shader_load(ItemShader *shader);

#endif
