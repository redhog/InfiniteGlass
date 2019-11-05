#include "glapi.h"
#include "xapi.h"
#include "item.h"
#include "item_window_shader.h"
#include "wm.h"
#include <limits.h>

void item_type_base_constructor(Item *item, void *args) {
  Window window = *(Window *) args;
  
  item->window = window;
  item->width_property = 0;
  item->height_property = 0;
  item->width_window = 0;
  item->height_window = 0;

  Atom type_return;
  int format_return;
  unsigned long nitems_return;
  unsigned long bytes_after_return;
  unsigned char *prop_return = NULL;
  XGetWindowProperty(display, window, IG_LAYER, 0, sizeof(Atom), 0, XA_ATOM,
                     &type_return, &format_return, &nitems_return, &bytes_after_return, &prop_return);
  if (type_return == None) {
    XWindowAttributes attr;
    XGetWindowAttributes(display, window, &attr);
    if (attr.override_redirect) {
      item->layer = IG_LAYER_MENU;
    } else {
      item->layer = IG_LAYER_DESKTOP;
    }
  } else {
    item->layer = *(Atom *) prop_return;
  }
  XFree(prop_return);

  long value = 1;
  XChangeProperty(display, window, WM_STATE, XA_INTEGER, 32, PropModeReplace, (void *) &value, 1);
  
  XWindowAttributes attr;
  XGetWindowAttributes(display, window, &attr);
  item->is_mapped = attr.map_state == IsViewable;
  item_type_window_update_space_pos_from_window(item);

  XSelectInput(display, window, PropertyChangeMask);

  item->properties = properties_load(window);
  item->prop_size = properties_find(item->properties, IG_SIZE);
  item->prop_coords = properties_find(item->properties, IG_COORDS);
}
void item_type_base_destructor(Item *item) {}
void item_type_base_draw(View *view, Item *item) {
  if (item->is_mapped) {
    ItemShader *shader = (ItemShader *) item->type->get_shader(item);

    properties_to_gl(item->properties, shader->shader);
    gl_check_error("item_draw1");
    
    glUniform1i(shader->picking_mode_attr, view->picking);
    glUniform4fv(shader->screen_attr, 1, view->screen);
    
    glUniform1f(shader->window_id_attr, (float) item->id / (float) INT_MAX);

    gl_check_error("item_draw2");
    
    glDrawArrays(GL_POINTS, 0, 1);

    gl_check_error("item_draw3");
  }
}
void item_type_base_update(Item *item) {
  if (!item->is_mapped) return;

  if (item->width != item->width_window || item->height != item->height_window) {
    XWindowChanges values;
    values.width = item->width;
    values.height = item->height;
    XConfigureWindow(display, item->window, CWWidth | CWHeight, &values);
    item->width_window = item->width;
    item->height_window = item->height;
  }
  if (item->width != item->width_property || item->height != item->height_property) {
    long arr[2] = {item->width, item->height};
    XChangeProperty(display, item->window, IG_SIZE, XA_INTEGER, 32, PropModeReplace, (void *) arr, 2);
    item->width_property = item->width;
    item->height_property = item->height;
  }
  item->_is_mapped = item->is_mapped;
}

Shader *item_type_base_get_shader(Item *item) {
  return (Shader *) item_window_shader_get();
}

void item_type_base_print(Item *item) {
  printf("%s(%d):%s [%d,%d] @ %f,%f,%f,%f\n",
         item->type->name,
         item->id,
         item->is_mapped ? "" : " invisible",
         item->width,
         item->height,
         item->coords[0],
         item->coords[1],
         item->coords[2],
         item->coords[3]);
  printf("    window=%ld\n", item->window);
  properties_print(item->properties, stderr);
}

ItemType item_type_base = {
  NULL,
  sizeof(Item),
  "ItemBase",
  &item_type_base_constructor,
  &item_type_base_destructor,
  &item_type_base_draw,
  &item_type_base_update,
  &item_type_base_get_shader,
  &item_type_base_print
};

List *items_all = NULL;
size_t items_all_id = 0;

Bool item_isinstance(Item *item, ItemType *type) {
  ItemType *item_type;
  for (item_type = item->type; item_type && item_type != type; item_type = item_type->base);
  return !!item_type;
}

Item *item_create(ItemType *type, void *args) {
  Item *item = (Item *) malloc(type->size);

  item->width = 0;
  item->height = 0;
  item->coords[0] = 0.0;
  item->coords[1] = 0.0;
  item->coords[2] = 0.0;
  item->coords[3] = 0.0;
  item->is_mapped = False;
  item->_is_mapped = False;
  item->type = type;
  item->type->init(item, args);
  item_add(item);
  item->type->update(item);
  return item;
}

Item *item_get(int id) {
  if (items_all) {
    for (size_t idx = 0; idx < items_all->count; idx++) {
      Item *item = (Item *) items_all->entries[idx];
      if (item->id == id) return item;
    }
  }
  return NULL;
}

void item_add(Item *item) {
  if (!items_all) items_all = list_create();
  item->id = ++items_all_id;  
  list_append(items_all, (void *) item);
}

void item_remove(Item *item) {
  list_remove(items_all, (void *) item);
  item->type->destroy(item);
}
