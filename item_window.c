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

void item_type_window_update_texture(WindowItem *item) {
  if (!item->base.is_mapped) return;
  x_push_error_context("item_update_texture");
  if (!item->window_texture_id) {
    glGenTextures(1, &item->window_texture_id);
    gl_check_error("item_update_texture1");
  }
  glBindTexture(GL_TEXTURE_2D, item->window_texture_id);
  gl_check_error("item_update_texture3");
  glXBindTexImageEXT(display, item->window_glxpixmap, GLX_FRONT_EXT, NULL);
  gl_check_error("item_update_texture4");
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  gl_check_error("item_update_texture5");
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  gl_check_error("item_update_texture6");

  if (item->wm_hints.flags & IconPixmapHint) {
    if (!item->icon_texture_id) {
      glGenTextures(1, &item->icon_texture_id);
      gl_check_error("item_update_texture7");
    }
    glBindTexture(GL_TEXTURE_2D, item->icon_texture_id);
    gl_check_error("item_update_texture9");
    glXBindTexImageEXT(display, item->icon_glxpixmap, GLX_FRONT_EXT, NULL);
    gl_check_error("item_update_texture10");
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    gl_check_error("item_update_texture11");
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    gl_check_error("item_update_texture12");
  }
  x_pop_error_context();
}

void item_type_window_destructor(Item *item) {}
void item_type_window_draw(Item *item) {
  if (item->is_mapped) {
    WindowItem *window_item = (WindowItem *) item;
    
    item_type_window_update_texture(window_item);

    glUniform1i(window_sampler_attr, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, window_item->window_texture_id);
    glBindSampler(1, 0);

    if (window_item->wm_hints.flags & IconPixmapHint) {
      glUniform1i(icon_sampler_attr, 1);
      glActiveTexture(GL_TEXTURE1);
      glBindTexture(GL_TEXTURE_2D, window_item->icon_texture_id);
      glBindSampler(1, 0);
    }
    
    item_type_base.draw(item);
  }
}

void item_type_window_update(Item *item) {
  WindowItem *window_item = (WindowItem *) item;
   
  if (!item->is_mapped) return;

  item_type_base.update(item);

  x_push_error_context("item_update_pixmap");
  if (window_item->window_pixmap) XFreePixmap(display, window_item->window_pixmap);
  if (window_item->window_glxpixmap) glXDestroyGLXPixmap(display, window_item->window_glxpixmap);
  gl_check_error("item_update_pixmap0");

  
  // FIXME: free all other stuff if already created

  window_item->window_pixmap = XCompositeNameWindowPixmap(display, window_item->window);
  const int pixmap_attribs[] = {
    GLX_TEXTURE_TARGET_EXT, GLX_TEXTURE_2D_EXT,
    GLX_TEXTURE_FORMAT_EXT, GLX_TEXTURE_FORMAT_RGB_EXT,
    None
  };
  gl_check_error("item_update_pixmap1");

  if (window_item->wm_hints.flags & IconPixmapHint) {
    printf("Window %d: PIXMAP: %d: %d\n",
           window_item->window,
           window_item->wm_hints.flags & IconPixmapHint,
           window_item->wm_hints.icon_pixmap);
    window_item->icon_glxpixmap = glXCreatePixmap(display, configs[0], window_item->wm_hints.icon_pixmap, pixmap_attribs);
  }
  window_item->window_glxpixmap = glXCreatePixmap(display, configs[0], window_item->window_pixmap, pixmap_attribs);

  gl_check_error("item_update_pixmap2");

  item_type_window_update_texture(window_item);
  gl_check_error("item_update_pixmap3");
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
  item->window_glxpixmap = 0;
  item->window_texture_id = 0;
  item->icon_glxpixmap = 0;
  item->icon_texture_id = 0;
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
