#ifndef ITEM_WIDGET
#define ITEM_WIDGET

#include "item.h"
#include "view.h"
#include "texture.h"
#include <cairo.h>

typedef struct {
  // x and y are ]0,1[, from top left to bottom right of window.
  // width and height are ]0,1[ as a fraction of the whole window width/height
  float x;
  float y;
  float width;
  float height;
  int pixelwidth;
  int pixelheight;
  int itempixelwidth;
  int itempixelheight;
  cairo_surface_t *surface;
  Texture texture;
} WidgetItemTile;

#define WIDGET_ITEM_TILE_CACHE_SIZE 5

typedef struct {
  Item base;
  char *label;
  WidgetItemTile tiles[WIDGET_ITEM_TILE_CACHE_SIZE];
} WidgetItem;

extern ItemType item_type_widget;

extern Item *item_get_widget(char *label);

#endif
