#ifndef ITEM_WIDGET_SHADER
#define ITEM_WIDGET_SHADER

#include "item_shader.h"

typedef struct {
  ItemShader base;
 
  GLint texture_attr;
  GLint transform_attr;
} ItemWidgetShader;

int item_widget_shader_load(ItemWidgetShader *shader);
ItemWidgetShader *item_widget_shader_get();

#endif
