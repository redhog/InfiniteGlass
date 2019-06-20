#ifndef ITEM
#define ITEM

#include "xapi.h"
#include "glapi.h"

struct ItemStruct;
typedef struct ItemStruct Item;
typedef void ItemTypeDestructor(Item *item);
typedef void ItemTypeDraw(Item *item);
typedef void ItemTypeUpdate(Item *item);
typedef struct {
  ItemTypeDestructor *destructor;
  ItemTypeDraw *draw;
  ItemTypeUpdate *update;
} ItemType;

struct ItemStruct {
  ItemType *type;

  int id;
 
  int width;
  int height;

  float coords[4];
  GLuint coords_vbo;

  uint is_mapped; 

  int _width;
  int _height;

  float _coords[4];
  uint _is_mapped; 
};

extern ItemType item_type_base;

extern Item **items_all;
extern size_t items_all_usage;

Item *item_get(int id);
void item_add(Item *item);
void item_remove(Item *item);

#endif
