#ifndef ITEM
#define ITEM

#include "xapi.h"
#include "glapi.h"
#include "item_shader.h"
#include "list.h"
#include "view_type.h"
#include "texture.h"
#include "property.h"
#include "rendering.h"

struct ItemTypeStruct;
struct ItemStruct;

typedef struct ItemTypeStruct ItemType;
typedef struct ItemStruct Item;

typedef void ItemTypeConstructor(Item *item, void *args);
typedef void ItemTypeDestructor(Item *item);
typedef void ItemTypeDraw(Rendering *rendering);
typedef void ItemTypeUpdate(Item *item);
typedef ItemShader *ItemTypeGetShader(Item *);
typedef void ItemTypePrint(Item *);

struct ItemTypeStruct {
  ItemType *base;
  size_t size;
  char *name;
  ItemTypeConstructor *init;
  ItemTypeDestructor *destroy;
  ItemTypeDraw *draw;
  ItemTypeUpdate *update;
  ItemTypeGetShader *get_shader;
  ItemTypePrint *print;
};

struct ItemStruct {
  ItemType *type;

  int id;
 
  uint is_mapped; 

  Atom layer;
 
  uint _is_mapped;

  int x;
  int y;
  Window window;

  List *properties;
  Property *prop_size;
  Property *prop_coords;
};

extern ItemType item_type_base;

extern List *items_all;

Bool item_isinstance(Item *item, ItemType *type);
Item *item_create(ItemType *type, void *args);
Item *item_get(int id);
void item_add(Item *item);
void item_remove(Item *item);

extern void item_type_window_update_space_pos_from_window(Item *item);
extern Item *item_get_from_window(Window window, int create);
extern void items_get_from_toplevel_windows();

#endif
