#include "glapi.h"
#include "item_window.h"
#include "item_window_shader.h"
#include "item_window_pixmap.h"
#include "item_window_svg.h"
#include "xapi.h"
#include "wm.h"
#include <X11/Xatom.h>

void item_type_window_update_space_pos_from_window(ItemWindow *item) {
  XWindowAttributes attr;
  XGetWindowAttributes(display, item->window, &attr);

  int x                     = attr.x;
  int y                     = attr.y;
  int width                 = attr.width;
  int height                = attr.height;
  // fprintf(stderr, "Spacepos for %ld is %d,%d [%d,%d]\n", item->window, x, y, width, height);

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
     item->base._coords[i] = *(float *) (i + (long *) prop_return);
    }
    XFree(prop_return);
  } else {
    item->base.coords[0] = ((float) (x - overlay_attr.x)) / (float) overlay_attr.width;
    item->base.coords[1] = ((float) (overlay_attr.height - y - overlay_attr.y)) / (float) overlay_attr.width;
    item->base.coords[2] = ((float) (width)) / (float) overlay_attr.width;
    item->base.coords[3] = ((float) (height)) / (float) overlay_attr.width;
  }
}

void item_type_window_constructor(Item *item, void *args) {
  ItemWindow *window_item = (ItemWindow *) item;
  Window window = *(Window *) args;
  
  window_item->window = window;

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

  XSelectInput(display, window, PointerMotionMask | PropertyChangeMask);
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

  if (   (item->_coords[0] != item->coords[0])
      || (item->_coords[1] != item->coords[1])
      || (item->_coords[2] != item->coords[2])
      || (item->_coords[3] != item->coords[3])) {
    long arr[4];
    for (int i = 0; i < 4; i++) {
     arr[i] = *(long *) &item->coords[i];
    }
    XChangeProperty(display, window_item->window, IG_COORDS, XA_FLOAT, 32, PropModeReplace, (void *) arr, 4);
  }
  if (item->width != item->_width || item->height != item->_height) {
    XWindowChanges values;
    values.width = item->width;
    values.height = item->height;
    XConfigureWindow(display, window_item->window, CWWidth | CWHeight, &values);
    long arr[2] = {item->width, item->height};
    XChangeProperty(display, window_item->window, IG_SIZE, XA_INTEGER, 32, PropModeReplace, (void *) arr, 2);
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
    ItemWindowSVGArgs args = {window, prop_return};
    return item_create(&item_type_window_svg, &args);
  }
}


void items_get_from_toplevel_windows() {
  XWindowAttributes attr;
  
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
    if (top_level_windows[i] == motion_notification_window) continue;
    item_get_from_window(top_level_windows[i]);
  }

  XFree(top_level_windows);
  XUngrabServer(display);
}
