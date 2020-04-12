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

struct ItemStruct;
typedef struct ItemStruct Item;

struct ItemStruct {
  Window window;
  xcb_get_window_attributes_reply_t *attr;
  xcb_get_geometry_reply_t *geom;
 
  uint is_mapped; 
  uint _is_mapped;

  int x;
  int y;

  Properties *properties;
  Property *prop_layer;
  Property *prop_item_layer;
  Property *prop_shader;
  Property *prop_size;
  Property *prop_coords;
  Property *prop_coord_types;
  Property *prop_draw_type;

  Damage damage;
 
  Pixmap window_pixmap;
  Texture window_texture;

  int draw_cycles_left;
 
  Item *parent_item; // Only used as caching for picking. DO NOT USE outside of that, value might change at any time.
};

extern List *items_all;
extern Item *root_item;

extern Bool init_items();

extern Item *item_create(Window window);
extern void item_add(Item *item);
extern void item_remove(Item *item);

extern void item_draw(Rendering *rendering);
extern void item_update(Item *item);
extern void item_properties_update(Item *item, Atom name);
extern Shader *item_get_shader(Item *item);
extern void item_print(Item *);

extern void item_update_space_pos_from_window(Item *item);
extern Item *item_get_from_window(Window window, int create);
extern Item *item_get_from_widget(Item *parent, int widget);
extern void items_get_from_toplevel_windows();

#endif
