#ifndef ITEM_WIDGET
#define ITEM_WIDGET

#include "item.h"
#include "view.h"
#include "texture.h"
#include <cairo.h>

typedef struct {
  Item base;
  char *label;
  cairo_surface_t *surface;
  Texture texture;
} WidgetItem;

extern ItemType item_type_widget;

extern Item *item_get_widget(char *label);

#endif
