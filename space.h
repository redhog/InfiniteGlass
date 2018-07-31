#ifndef SHAPE
#define SHAPE

#include "xapi.h"
#include "glapi.h"

typedef struct {
  Window window;

  Damage damage;
 
  Pixmap pixmap;
  GLXPixmap glxpixmap;
  GLuint texture_id;

  float coords[4];
  GLuint coords_vbo;

  uint is_mapped;
} Item;

Item **items_all;

Item *item_get(Window window);
void item_remove(Item *item);
void item_update_space_pos_from_window(Item *item);
void item_update_space_pos(Item *item);
void item_update_pixmap(Item *item);
void item_update_texture(Item *item);

#endif
