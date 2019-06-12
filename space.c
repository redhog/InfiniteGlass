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
 item->pixmap = 0;
 item->glxpixmap = 0;
 item->texture_id = 0;
 item->window = window;
 item->damage = XDamageCreate(display, window, XDamageReportNonEmpty);
 items_all[idx] = item;
 items_all[idx+1] = NULL;
 
 return item;
}

void item_remove(Item *item) {
 size_t idx;
 
 for (idx = 0; items_all[idx] && items_all[idx] != item; idx++);
 if (!items_all[idx]) return;
 memmove(items_all+idx, items_all+idx+1, items_all_size-idx-1);
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
 if (item->pixmap) XFreePixmap(display, item->pixmap);
 if (item->glxpixmap) glXDestroyGLXPixmap(display, item->glxpixmap);
 checkError("item_update_pixmap0");
 
 // FIXME: free all other stuff if already created
 
 item->pixmap = XCompositeNameWindowPixmap(display, item->window);
  const int pixmap_attribs[] = {
   GLX_TEXTURE_TARGET_EXT, GLX_TEXTURE_2D_EXT,
   GLX_TEXTURE_FORMAT_EXT, GLX_TEXTURE_FORMAT_RGB_EXT,
   None
  };
 checkError("item_update_pixmap1");
 item->glxpixmap = glXCreatePixmap(display, configs[0], item->pixmap, pixmap_attribs);
 checkError("item_update_pixmap2");
 
 item_update_texture(item);
 checkError("item_update_pixmap3");
}

void item_update_texture(Item *item) {
 if (!item->is_mapped) return;
 //glEnable(GL_TEXTURE_2D);
 //checkError("item_update_texture1");
 if (!item->texture_id) {
   glGenTextures(1, &item->texture_id);
 }
 checkError("item_update_texture2");
 glBindTexture(GL_TEXTURE_2D, item->texture_id);
 checkError("item_update_texture3");
 glXBindTexImageEXT(display, item->glxpixmap, GLX_FRONT_EXT, NULL);
 checkError("item_update_texture4");
 glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
 checkError("item_update_texture5");
 glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
 checkError("item_update_texture6");
 // glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
 // checkError("item_update_texture7");
}
