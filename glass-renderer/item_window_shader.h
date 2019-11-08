#ifndef ITEM_WINDOW_SHADER
#define ITEM_WINDOW_SHADER

#include "item_shader.h"

typedef struct {
  ItemShader base;
} ItemWindowShader;

int item_window_shader_load(ItemWindowShader *shader);
ItemWindowShader *item_window_shader_get();

#endif
