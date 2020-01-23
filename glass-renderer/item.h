#ifndef ITEM
#define ITEM

#include "xapi.h"
#include "glapi.h"
#include "shader.h"
#include "list.h"
#include "view_type.h"
#include "texture.h"
#include "property.h"
#include "rendering.h"

extern Atom IG_DRAW_TYPE;

struct ItemStruct;
typedef struct ItemStruct Item;

struct ItemStruct {
  int id;
  Window window;
  XWindowAttributes attr;
 
  uint is_mapped; 
  uint _is_mapped;

  int x;
  int y;

  Properties *properties;
  Property *prop_layer;
  Property *prop_shader;
  Property *prop_size;
  Property *prop_coords;
  Property *prop_draw_type;

  Damage damage;
 
  Pixmap window_pixmap;
  Texture window_texture;
};

extern List *items_all;
extern Item *root_item;

extern Bool init_items();

extern Item *item_create(Window window);
extern Item *item_get(int id);
extern void item_add(Item *item);
extern void item_remove(Item *item);

extern void item_draw(Rendering *rendering);
extern void item_update(Item *item);
extern Shader *item_get_shader(Item *item);
extern void item_print(Item *);

extern void item_update_space_pos_from_window(Item *item);
extern Item *item_get_from_window(Window window, int create);
extern void items_get_from_toplevel_windows();

#endif
