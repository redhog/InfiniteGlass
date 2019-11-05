#ifndef ITEM
#define ITEM

#include "xapi.h"
#include "glapi.h"
#include "item_shader.h"
#include "list.h"
#include "view_type.h"

struct ItemTypeStruct;
struct ItemStruct;

typedef struct ItemTypeStruct ItemType;
typedef struct ItemStruct Item;

typedef void ItemTypeConstructor(Item *item, void *args);
typedef void ItemTypeDestructor(Item *item);
typedef void ItemTypeDraw(View *view, Item *item);
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
 
  int width;
  int height;

  float coords[4];

  uint is_mapped; 

  int layer;
 
//  float _coords[4];
  uint _is_mapped; 
};

extern ItemType item_type_base;

extern List *items_all;

Bool item_isinstance(Item *item, ItemType *type);
Item *item_create(ItemType *type, void *args);
Item *item_get(int id);
void item_add(Item *item);
void item_remove(Item *item);

#endif
