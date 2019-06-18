#include "glapi.h"
#include "space.h"
#include "xapi.h"

Item **items_all = NULL;
size_t items_all_size = 0;

Item *item_get(Window window) {
  Item *item;
  size_t idx = 0;

  if (items_all) {
    for (; items_all[idx] && items_all[idx]->window != window; idx++);
    if (items_all[idx]) return items_all[idx];
  }

  fprintf(stderr, "Adding window %ld\n", window);

  if (idx+1 > items_all_size) {
   if (!items_all_size) items_all_size = 8;
   items_all_size *=2;
   items_all = realloc(items_all, sizeof(Item *) * items_all_size);
  }

  item = (Item *) malloc(sizeof(Item));
  item->coords_vbo = -1;
  item->is_mapped = False;
  item->window_pixmap = 0;
  item->window_glxpixmap = 0;
  item->window_texture_id = 0;
  item->icon_glxpixmap = 0;
  item->icon_texture_id = 0;
  item->window = window;
  item->damage = XDamageCreate(display, window, XDamageReportNonEmpty);
  items_all[idx] = item;
  items_all[idx+1] = NULL;

  item->wm_hints.flags = 0;

  XWMHints *wm_hints = XGetWMHints(display, window);
  if (wm_hints) {
    item->wm_hints = *wm_hints;
    XFree(wm_hints);
  }

  return item;
}

void item_remove(Item *item) {
  size_t idx;

  for (idx = 0; items_all[idx] && items_all[idx] != item; idx++);
  if (!items_all[idx]) return;
  memmove(items_all+idx, items_all+idx+1, sizeof(Item *) * (items_all_size-idx-1));
}

void item_update_space_pos_from_window(Item *item) {
  XWindowAttributes attr;
  XGetWindowAttributes(display, item->window, &attr);

  int x                     = attr.x;
  int y                     = attr.y;
  int width                 = attr.width;
  int height                = attr.height;
  fprintf(stderr, "Spacepos for %ld is %d,%d [%d,%d]\n", item->window, x, y, width, height);

  item->width = width;
  item->height = height;
  
  item->coords[0] = ((float) (x - overlay_attr.x)) / (float) overlay_attr.width;
  item->coords[1] = ((float) (overlay_attr.height - y - overlay_attr.y)) / (float) overlay_attr.width;
  item->coords[2] = ((float) (width)) / (float) overlay_attr.width;
  item->coords[3] = ((float) (height)) / (float) overlay_attr.width;

  item_update_space_pos(item);
}

void item_update_space_pos(Item *item) {
  if (item->coords_vbo == -1) {
    glGenBuffers(1, &item->coords_vbo);
  }
  glBindBuffer(GL_ARRAY_BUFFER, item->coords_vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(item->coords), item->coords, GL_STATIC_DRAW);
}

void item_update_pixmap(Item *item) {
  if (!item->is_mapped) return;
  x_push_error_context("item_update_pixmap");
  if (item->window_pixmap) XFreePixmap(display, item->window_pixmap);
  if (item->window_glxpixmap) glXDestroyGLXPixmap(display, item->window_glxpixmap);
  gl_check_error("item_update_pixmap0");

  // FIXME: free all other stuff if already created

  item->window_pixmap = XCompositeNameWindowPixmap(display, item->window);
  const int pixmap_attribs[] = {
    GLX_TEXTURE_TARGET_EXT, GLX_TEXTURE_2D_EXT,
    GLX_TEXTURE_FORMAT_EXT, GLX_TEXTURE_FORMAT_RGB_EXT,
    None
  };
  gl_check_error("item_update_pixmap1");

  if (item->wm_hints.flags & IconPixmapHint) {
    printf("Window %d: PIXMAP: %d: %d\n",
           item->window,
           item->wm_hints.flags & IconPixmapHint,
           item->wm_hints.icon_pixmap);
    item->icon_glxpixmap = glXCreatePixmap(display, configs[0], item->wm_hints.icon_pixmap, pixmap_attribs);
  }
  item->window_glxpixmap = glXCreatePixmap(display, configs[0], item->window_pixmap, pixmap_attribs);

  gl_check_error("item_update_pixmap2");

  item_update_texture(item);
  gl_check_error("item_update_pixmap3");
  x_pop_error_context();
}

void item_update_texture(Item *item) {
  if (!item->is_mapped) return;
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
      printf("CREATE Window %d, icon %d\n", item->window, item->icon_texture_id);
    }
    printf("Window %d, icon %d\n", item->window, item->icon_texture_id);
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
