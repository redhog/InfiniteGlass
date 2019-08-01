#include "glapi.h"
#include "item_window.h"
#include "item_window_shader.h"
#include "item_window_pixmap.h"
#include "xapi.h"
#include "wm.h"

void item_type_window_update_space_pos_from_window(ItemWindow *item) {
  XWindowAttributes attr;
  XGetWindowAttributes(display, item->window, &attr);

  int x                     = attr.x;
  int y                     = attr.y;
  int width                 = attr.width;
  int height                = attr.height;
  fprintf(stderr, "Spacepos for %ld is %d,%d [%d,%d]\n", item->window, x, y, width, height);

  item->base.width = width;
  item->base.height = height;
  
  item->base.coords[0] = ((float) (x - overlay_attr.x)) / (float) overlay_attr.width;
  item->base.coords[1] = ((float) (overlay_attr.height - y - overlay_attr.y)) / (float) overlay_attr.width;
  item->base.coords[2] = ((float) (width)) / (float) overlay_attr.width;
  item->base.coords[3] = ((float) (height)) / (float) overlay_attr.width;

  item_type_window.base->update((Item *) item);
}

void item_type_window_constructor(Item *item, void *args) {
  ItemWindow *window_item = (ItemWindow *) item;
  Window window = *(Window *) args;
  
  window_item->window = window;

  XWindowAttributes attr;
  XGetWindowAttributes(display, window, &attr);
  window_item->base.is_mapped = attr.map_state == IsViewable;
  item_type_window_update_space_pos_from_window(item);
}

void item_type_window_destructor(Item *item) {
  item_type_base.destroy(item);
}

void item_type_window_draw(View *view, Item *item) {
  item_type_base.draw(view, item);
}

void item_type_window_update(Item *item) {
  ItemWindow *window_item = (ItemWindow *) item;
   
  if (!item->is_mapped) return;

  if (item->width != item->_width || item->height != item->_height) {
    XWindowChanges values;
    values.width = item->width;
    values.height = item->height;
    XConfigureWindow(display, window_item->window, CWWidth | CWHeight, &values);
  }
  
  item_type_window.base->update(item);
}

Shader *item_type_window_get_shader(Item *item) {
  return (Shader *) item_window_shader_get();
}

ItemType item_type_window = {
  &item_type_base,
  sizeof(ItemWindow),
  &item_type_window_constructor,
  &item_type_window_destructor,
  &item_type_window_draw,
  &item_type_window_update,
  &item_type_window_get_shader
};

Item *item_get_from_window(Window window) {
  ItemWindow *item;
  size_t idx = 0;

  if (items_all) {
    for (;
         items_all[idx]
         && (   !item_isinstance(items_all[idx], &item_type_window)
             || ((ItemWindow *) items_all[idx])->window != window);
         idx++);
    if (items_all[idx]) return items_all[idx];
  }

  fprintf(stderr, "Adding window %ld\n", window);

  return item_create(&item_type_window_pixmap, &window);
}
