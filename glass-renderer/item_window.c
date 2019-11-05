#include "glapi.h"
#include "item_window.h"
#include "item_window_shader.h"
#include "item_window_pixmap.h"
#include "item_window_svg.h"
#include "property.h"
#include "xapi.h"
#include "wm.h"
#include "debug.h"
#include <X11/Xatom.h>

void item_type_window_update_space_pos_from_window(ItemWindow *item) {
  XWindowAttributes attr;
  XGetWindowAttributes(display, item->window, &attr);

  item->x                   = attr.x;
  item->y                   = attr.y;
  int width                 = attr.width;
  int height                = attr.height;
  DEBUG("window.spacepos", "Spacepos for %ld is %d,%d [%d,%d]\n", item->window, item->x, item->y, width, height);

  item->base.width = width;
  item->base.height = height;
  
  Atom type_return;
  int format_return;
  unsigned long nitems_return;
  unsigned long bytes_after_return;
  unsigned char *prop_return;
  XGetWindowProperty(display, item->window, IG_COORDS, 0, sizeof(float)*4, 0, AnyPropertyType,
                       &type_return, &format_return, &nitems_return, &bytes_after_return, &prop_return);
  if (type_return != None) {
    for (int i = 0; i < 4; i++) {
     item->base.coords[i] = *(float *) (i + (long *) prop_return);
    }
    XFree(prop_return);
  } else {
    View *v = NULL;
    if (views) {
      v = view_find(views, item->base.layer);
    }
    if (v) {
      item->base.coords[0] = v->screen[0] + (v->screen[2] * (float) item->x) / (float) v->width;
      item->base.coords[1] = v->screen[1] + v->screen[3] - (v->screen[3] * (float) item->y) / (float) v->height;
      item->base.coords[2] = (v->screen[2] * (float) item->base.width) / (float) v->width;
      item->base.coords[3] = (v->screen[3] * (float) item->base.height) / (float) v->height;
    } else {
      item->base.coords[0] = ((float) (item->x - overlay_attr.x)) / (float) overlay_attr.width;
      item->base.coords[1] = ((float) (overlay_attr.height - item->y - overlay_attr.y)) / (float) overlay_attr.width;
      item->base.coords[2] = ((float) (width)) / (float) overlay_attr.width;
      item->base.coords[3] = ((float) (height)) / (float) overlay_attr.width;
    }
    long arr[4];
    for (int i = 0; i < 4; i++) {
     arr[i] = *(long *) &item->base.coords[i];
    }
    XChangeProperty(display, item->window, IG_COORDS, XA_FLOAT, 32, PropModeReplace, (void *) arr, 4);
  }
}

void item_type_window_constructor(Item *item, void *args) {
  ItemWindow *window_item = (ItemWindow *) item;
  Window window = *(Window *) args;
  
  window_item->window = window;
  window_item->width_property = 0;
  window_item->height_property = 0;
  window_item->width_window = 0;
  window_item->height_window = 0;

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
      window_item->base.layer = IG_LAYER_MENU;
    } else {
      window_item->base.layer = IG_LAYER_DESKTOP;
    }
  } else {
    window_item->base.layer = *(Atom *) prop_return;
  }
  XFree(prop_return);

  long value = 1;
  XChangeProperty(display, window, WM_STATE, XA_INTEGER, 32, PropModeReplace, (void *) &value, 1);
  
  XWindowAttributes attr;
  XGetWindowAttributes(display, window, &attr);
  window_item->base.is_mapped = attr.map_state == IsViewable;
  item_type_window_update_space_pos_from_window(window_item);

  XSelectInput(display, window, PropertyChangeMask);

  window_item->properties = properties_load(window);
  window_item->prop_size = properties_find(window_item->properties, IG_SIZE);
  window_item->prop_coords = properties_find(window_item->properties, IG_COORDS);
}

void item_type_window_destructor(Item *item) {
  item_type_base.destroy(item);
}

void item_type_window_draw(View *view, Item *item) {
  properties_to_gl(((ItemWindow *) item)->properties, item->type->get_shader(item)->shader);
  gl_check_error("item_type_window_draw1");
  item_type_base.draw(view, item);
  gl_check_error("item_type_window_draw2");
}

void item_type_window_update(Item *item) {
  ItemWindow *window_item = (ItemWindow *) item;
   
  if (!item->is_mapped) return;

  if (item->width != window_item->width_window || item->height != window_item->height_window) {
    XWindowChanges values;
    values.width = item->width;
    values.height = item->height;
    XConfigureWindow(display, window_item->window, CWWidth | CWHeight, &values);
    window_item->width_window = item->width;
    window_item->height_window = item->height;
  }
  if (item->width != window_item->width_property || item->height != window_item->height_property) {
    long arr[2] = {item->width, item->height};
    XChangeProperty(display, window_item->window, IG_SIZE, XA_INTEGER, 32, PropModeReplace, (void *) arr, 2);
    window_item->width_property = item->width;
    window_item->height_property = item->height;
  }
  item_type_window.base->update(item);
}

Shader *item_type_window_get_shader(Item *item) {
  return (Shader *) item_window_shader_get();
}

void item_type_window_print(Item *item) {
  item_type_window.base->print(item);
  printf("    window=%ld\n", ((ItemWindow *) item)->window);
  properties_print(((ItemWindow *) item)->properties, stderr);
}

ItemType item_type_window = {
  &item_type_base,
  sizeof(ItemWindow),
  "ItemWindow",
  &item_type_window_constructor,
  &item_type_window_destructor,
  &item_type_window_draw,
  &item_type_window_update,
  &item_type_window_get_shader,
  &item_type_window_print
};

Item *item_get_from_window(Window window, int create) {
  if (items_all) {
    for (size_t idx = 0; idx < items_all->count; idx++) {
      ItemWindow *item = (ItemWindow *) items_all->entries[idx];
      if (item->window == window) return (Item *) item;
    }
  }
  if (!create) return NULL;
  
  DEBUG("window.add", "Adding window %ld\n", window);

  Atom type_return;
  int format_return;
  unsigned long nitems_return;
  unsigned long bytes_after_return;
  unsigned char *prop_return;
  
  XGetWindowProperty(display, window, DISPLAYSVG, 0, 0, 0, XA_STRING, &type_return, &format_return, &nitems_return, &bytes_after_return, &prop_return);
  XFree(prop_return);
  if (type_return == None) {
    return item_create(&item_type_window_pixmap, &window);
  } else {
    XGetWindowProperty(display, window, DISPLAYSVG, 0, bytes_after_return, 0, XA_STRING, &type_return, &format_return, &nitems_return, &bytes_after_return, &prop_return);
    ItemWindowSVGArgs args = {window, (char *) prop_return};
    return item_create(&item_type_window_svg, &args);
  }
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
