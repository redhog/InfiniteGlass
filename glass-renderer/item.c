#include "glapi.h"
#include "xapi.h"
#include "item.h"
#include "view.h"
#include "item_window_shader.h"
#include "wm.h"
#include <limits.h>
#include "item_window_pixmap.h"
#include "property.h"
#include "debug.h"
#include "rendering.h"
#include <X11/Xatom.h>


void item_type_base_constructor(Item *item, void *args) {
  Window window = *(Window *) args;
  
  item->window = window;

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
void item_type_base_draw(Rendering *rendering) {
  if (rendering->item->is_mapped) {
    ItemShader *shader = (ItemShader *) rendering->item->type->get_shader(rendering->item);
    
    properties_to_gl(rendering->item->properties, rendering);
    gl_check_error("item_draw1");
    
    glUniform1i(shader->picking_mode_attr, rendering->view->picking);
    glUniform4fv(shader->screen_attr, 1, rendering->view->screen);
    
    glUniform1f(shader->window_id_attr, (float) rendering->item->id / (float) INT_MAX);

    gl_check_error("item_draw2");
    
    glDrawArrays(GL_POINTS, 0, 1);

    gl_check_error("item_draw3");
  }
}
void item_type_base_update(Item *item) {
  if (!item->is_mapped) return;
  item->_is_mapped = item->is_mapped;
}

ItemShader *item_type_base_get_shader(Item *item) {
  return (ItemShader *) item_window_shader_get();
}

void item_type_base_print(Item *item) {
  float _coords[] = {-1.,-1.,-1.,-1.};
  float *coords = _coords;
  long width = -1, height = -1;
  if (item->prop_size) {
    width = item->prop_size->values.dwords[0];
    height = item->prop_size->values.dwords[1];
  }
  if (item->prop_coords) {
    coords = (float *) item->prop_coords->data;
  }
  printf("%s(%d):%s [%ld,%ld] @ %f,%f,%f,%f\n",
         item->type->name,
         item->id,
         item->is_mapped ? "" : " invisible",
         width,
         height,
         coords[0],
         coords[1],
         coords[2],
         coords[3]);
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

void item_type_window_update_space_pos_from_window(Item *item) {
  XWindowAttributes attr;
  XGetWindowAttributes(display, item->window, &attr);

  item->x                   = attr.x;
  item->y                   = attr.y;
  int width                 = attr.width;
  int height                = attr.height;
  DEBUG("window.spacepos", "Spacepos for %ld is %d,%d [%d,%d]\n", item->window, item->x, item->y, width, height);

  long arr[2] = {width, height};
  XChangeProperty(display, item->window, IG_SIZE, XA_INTEGER, 32, PropModeReplace, (void *) arr, 2);
  
  Atom type_return;
  int format_return;
  unsigned long nitems_return;
  unsigned long bytes_after_return;
  unsigned char *prop_return;
  XGetWindowProperty(display, item->window, IG_COORDS, 0, sizeof(float)*4, 0, AnyPropertyType,
                       &type_return, &format_return, &nitems_return, &bytes_after_return, &prop_return);
  if (type_return != None) {
    XFree(prop_return);
  } else {
    View *v = NULL;
    if (views) {
      v = view_find(views, item->layer);
    }
    float coords[4];
    if (v) {
      coords[0] = v->screen[0] + (v->screen[2] * (float) item->x) / (float) v->width;
      coords[1] = v->screen[1] + v->screen[3] - (v->screen[3] * (float) item->y) / (float) v->height;
      coords[2] = (v->screen[2] * (float) width) / (float) v->width;
      coords[3] = (v->screen[3] * (float) height) / (float) v->height;
    } else {
      coords[0] = ((float) (item->x - overlay_attr.x)) / (float) overlay_attr.width;
      coords[1] = ((float) (overlay_attr.height - item->y - overlay_attr.y)) / (float) overlay_attr.width;
      coords[2] = ((float) (width)) / (float) overlay_attr.width;
      coords[3] = ((float) (height)) / (float) overlay_attr.width;
    }
    long arr[4];
    for (int i = 0; i < 4; i++) {
      arr[i] = *(long *) &coords[i];
    }
    XChangeProperty(display, item->window, IG_COORDS, XA_FLOAT, 32, PropModeReplace, (void *) arr, 4);
  }
}

Item *item_get_from_window(Window window, int create) {
  if (items_all) {
    for (size_t idx = 0; idx < items_all->count; idx++) {
      Item *item = (Item *) items_all->entries[idx];
      if (item->window == window) return item;
    }
  }
  if (!create) return NULL;
  
  DEBUG("window.add", "Adding window %ld\n", window);
  return item_create(&item_type_window_pixmap, &window);
}


void items_get_from_toplevel_windows() {
  XCompositeRedirectSubwindows(display, root, CompositeRedirectAutomatic);

  XGrabServer(display);

  Window returned_root, returned_parent;
  Window* top_level_windows;
  unsigned int num_top_level_windows;
  XQueryTree(display,
             root,
             &returned_root,
             &returned_parent,
             &top_level_windows,
             &num_top_level_windows);

  for (unsigned int i = 0; i < num_top_level_windows; ++i) {
    XWindowAttributes attr;
    XGetWindowAttributes(display, top_level_windows[i], &attr);
    if (attr.map_state == IsViewable) {
      item_get_from_window(top_level_windows[i], True);
    }
  }

  XFree(top_level_windows);
  XUngrabServer(display);
}
