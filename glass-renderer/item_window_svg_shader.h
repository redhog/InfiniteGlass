#ifndef ITEM_WINDOW_SVG_SHADER
#define ITEM_WINDOW_SVG_SHADER

#include "item_shader.h"

typedef struct {
  ItemShader base;
 
  GLint texture_attr;
  GLint transform_attr;
} ItemWindowSvgShader;

int item_window_svg_shader_load(ItemWindowSvgShader *shader);
ItemWindowSvgShader *item_window_svg_shader_get();

#endif
