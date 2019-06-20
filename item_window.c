#include "glapi.h"
#include "item_window.h"
#include "xapi.h"
#include "wm.h"

void item_type_window_update_space_pos_from_window(WindowItem *item) {
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

  item_type_base.update((Item *) item);
}

void item_type_window_destructor(Item *item) {
  WindowItem *window_item = (WindowItem *) item;
  texture_destroy(&window_item->window_texture);
  texture_destroy(&window_item->icon_texture);
  texture_destroy(&window_item->icon_mask_texture);
}

void item_type_window_draw(Item *item) {
  if (item->is_mapped) {
    WindowItem *window_item = (WindowItem *) item;

    texture_from_pixmap(&window_item->window_texture, window_item->window_pixmap);

    glUniform1i(window_sampler_attr, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, window_item->window_texture.texture_id);
    glBindSampler(1, 0);
    
    if (window_item->wm_hints.flags & IconPixmapHint) {
      glUniform1i(has_icon_attr, 1);
      glUniform1i(icon_sampler_attr, 1);
      glActiveTexture(GL_TEXTURE1);
      glBindTexture(GL_TEXTURE_2D, window_item->icon_texture.texture_id);
      glBindSampler(1, 0);
    } else {
      glUniform1i(has_icon_attr, 0);
    }
    if (window_item->wm_hints.flags & IconMaskHint) {
      glUniform1i(has_icon_mask_attr, 1);
      glUniform1i(icon_mask_sampler_attr, 2);
      glActiveTexture(GL_TEXTURE2);
      glBindTexture(GL_TEXTURE_2D, window_item->icon_mask_texture.texture_id);
      glBindSampler(1, 0);
    } else {
      glUniform1i(has_icon_mask_attr, 1);
    }
    
    item_type_base.draw(item);
  }
}

void item_type_window_update(Item *item) {
  WindowItem *window_item = (WindowItem *) item;
   
  if (!item->is_mapped) return;

  if (item->width != item->_width || item->height != item->_height) {
    XWindowChanges values;
    values.width = item->width;
    values.height = item->height;
    XConfigureWindow(display, window_item->window, CWWidth | CWHeight, &values);
  }
  
  item_type_base.update(item);

  x_push_error_context("item_update_pixmap");
  
  // FIXME: free all other stuff if already created

  window_item->window_pixmap = XCompositeNameWindowPixmap(display, window_item->window);
  texture_from_pixmap(&window_item->window_texture, window_item->window_pixmap);

  if (window_item->wm_hints.flags & IconPixmapHint) {
    texture_from_pixmap(&window_item->icon_texture, window_item->wm_hints.icon_pixmap);
  }
  if (window_item->wm_hints.flags & IconMaskHint) {
    texture_from_pixmap(&window_item->icon_mask_texture, window_item->wm_hints.icon_mask);
  }
  gl_check_error("item_update_pixmap2");

  x_pop_error_context();
}

ItemType item_type_window = {
  &item_type_window_destructor,
  &item_type_window_draw,
  &item_type_window_update
};

Item *item_get_from_window(Window window) {
  WindowItem *item;
  size_t idx = 0;

  if (items_all) {
    for (;
         items_all[idx]
         && items_all[idx]->type == &item_type_window
         && ((WindowItem *) items_all[idx])->window != window;
         idx++);
    if (items_all[idx]) return items_all[idx];
  }

  fprintf(stderr, "Adding window %ld\n", window);


  item = (WindowItem *) malloc(sizeof(WindowItem));
  item->base.type = &item_type_window;
  item_add((Item *) item);

  item->window_pixmap = 0;
  texture_initialize(&item->window_texture);
  texture_initialize(&item->icon_texture);
  texture_initialize(&item->icon_mask_texture);
  item->window = window;
  item->damage = XDamageCreate(display, window, XDamageReportNonEmpty);
  item->wm_hints.flags = 0;

  XErrorEvent error;
  x_try();
  XWMHints *wm_hints = XGetWMHints(display, window);
  if (wm_hints) {
    item->wm_hints = *wm_hints;
    XFree(wm_hints);
  }
  if (!x_catch(&error)) {
    printf("Window does not have any WM_HINTS: %d", window);
  }

  XWindowAttributes attr;
  XGetWindowAttributes(display, window, &attr);
  item->base.is_mapped = attr.map_state == IsViewable;

  item_type_window_update_space_pos_from_window(item);
  item_type_window.update((Item*) item);
  
  return (Item*) item;
}
