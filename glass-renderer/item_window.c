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

void item_type_window_update_space_pos_from_window(Item *item) {
  XWindowAttributes attr;
  XGetWindowAttributes(display, item->window, &attr);

  item->x                   = attr.x;
  item->y                   = attr.y;
  int width                 = attr.width;
  int height                = attr.height;
  DEBUG("window.spacepos", "Spacepos for %ld is %d,%d [%d,%d]\n", item->window, item->x, item->y, width, height);

  item->width = width;
  item->height = height;
  
  Atom type_return;
  int format_return;
  unsigned long nitems_return;
  unsigned long bytes_after_return;
  unsigned char *prop_return;
  XGetWindowProperty(display, item->window, IG_COORDS, 0, sizeof(float)*4, 0, AnyPropertyType,
                       &type_return, &format_return, &nitems_return, &bytes_after_return, &prop_return);
  if (type_return != None) {
    for (int i = 0; i < 4; i++) {
     item->coords[i] = *(float *) (i + (long *) prop_return);
    }
    XFree(prop_return);
  } else {
    View *v = NULL;
    if (views) {
      v = view_find(views, item->layer);
    }
    if (v) {
      item->coords[0] = v->screen[0] + (v->screen[2] * (float) item->x) / (float) v->width;
      item->coords[1] = v->screen[1] + v->screen[3] - (v->screen[3] * (float) item->y) / (float) v->height;
      item->coords[2] = (v->screen[2] * (float) item->width) / (float) v->width;
      item->coords[3] = (v->screen[3] * (float) item->height) / (float) v->height;
    } else {
      item->coords[0] = ((float) (item->x - overlay_attr.x)) / (float) overlay_attr.width;
      item->coords[1] = ((float) (overlay_attr.height - item->y - overlay_attr.y)) / (float) overlay_attr.width;
      item->coords[2] = ((float) (width)) / (float) overlay_attr.width;
      item->coords[3] = ((float) (height)) / (float) overlay_attr.width;
    }
    long arr[4];
    for (int i = 0; i < 4; i++) {
     arr[i] = *(long *) &item->coords[i];
    }
    XChangeProperty(display, item->window, IG_COORDS, XA_FLOAT, 32, PropModeReplace, (void *) arr, 4);
  }
}

void item_type_window_constructor(Item *item, void *args) {
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

void item_type_window_destructor(Item *item) {
  item_type_base.destroy(item);
}

void item_type_window_draw(View *view, Item *item) {
  properties_to_gl(item->properties, item->type->get_shader(item)->shader);
  gl_check_error("item_type_window_draw1");
  item_type_base.draw(view, item);
  gl_check_error("item_type_window_draw2");
}

void item_type_window_update(Item *item) {
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
  item_type_window.base->update(item);
}

Shader *item_type_window_get_shader(Item *item) {
  return (Shader *) item_window_shader_get();
}

void item_type_window_print(Item *item) {
  item_type_window.base->print(item);
  printf("    window=%ld\n", item->window);
  properties_print(item->properties, stderr);
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
      Item *item = (Item *) items_all->entries[idx];
      if (item->window == window) return item;
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
