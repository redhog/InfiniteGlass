#ifndef ITEM_WINDOW
#define ITEM_WINDOW

#include "item.h"

typedef struct {
  Item base;
 
  Window window;

  Damage damage;
 
  Pixmap window_pixmap;
  GLXPixmap window_glxpixmap;
  GLuint window_texture_id;

  GLXPixmap icon_glxpixmap;
  GLuint icon_texture_id;

  XWMHints wm_hints;
} WindowItem;

extern ItemType item_type_window;

Item *item_get_from_window(Window window);

#endif
