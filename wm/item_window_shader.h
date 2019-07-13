#ifndef ITEM_WINDOW_SHADER
#define ITEM_WINDOW_SHADER

#include "item_shader.h"

typedef struct {
  ItemShader base;
 
  GLint window_sampler_attr;
  GLint icon_sampler_attr;
  GLint icon_mask_sampler_attr;
  GLint has_icon_attr;
  GLint has_icon_mask_attr;
} ItemWindowShader;

int item_window_shader_load(ItemWindowShader *shader);
ItemWindowShader *item_window_shader_get();

#endif
