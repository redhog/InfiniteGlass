#ifndef ITEM_WINDOW_SVG
#define ITEM_WINDOW_SVG

#include "item.h"
#include "view.h"
#include "texture.h"
#include <cairo.h>

typedef struct {
  Window window;
  char *source;
} ItemWindowSVGArgs;

typedef struct {
  int x;
  int y;
  int width;
  int height;
  int itemwidth;
  int itemheight;
  cairo_surface_t *surface;
  Texture texture;
} ItemWindowSVGDrawing;

typedef struct {
  Item base;
 
  char *source;
 
  ItemWindowSVGDrawing drawing;
} ItemWindowSVG;

extern ItemType item_type_window_svg;

#endif
