#ifndef ITEM_WIDGET
#define ITEM_WIDGET

#include "item.h"
#include "view.h"
#include "texture.h"
#include <Imlib2.h>

typedef struct {
  Item base;
  char *label;
  char *font;
  Imlib_Image image;
  Texture texture;
} WidgetItem;

extern ItemType item_type_widget;

extern Item *item_get_widget(char *label, char *font);

#endif
