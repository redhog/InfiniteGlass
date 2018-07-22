#include "space.h"
#include "xapi.h"

Item **items_all = NULL;
size_t items_all_size = 0;

Item *item_get(Window window) {
 Item *item;
 size_t idx;
 
 for (idx = 0; items_all[idx] && items_all[idx]->window != window; idx++);
 if (items_all[idx]) return items_all[idx];

 if (idx+1 > items_all_size) {
  items_all_size *=2;
  items_all = realloc(items_all, sizeof(Item *) * items_all_size);
 }

 item = (Item *) malloc(sizeof(Item));
 item->window = window;
 item_update_texture(item);
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

void item_update_texture(Item *item) {
 item->pixmap = XCompositeNameWindowPixmap(display, window);
  const int pixmap_attribs[] = {
   GLX_TEXTURE_TARGET_EXT, GLX_TEXTURE_2D_EXT,
   GLX_TEXTURE_FORMAT_EXT, GLX_TEXTURE_FORMAT_RGB_EXT,
   None
  };
 item->glxpixmap = glXCreatePixmap(display, configs[0], item->pixmap, pixmap_attribs);
 glEnable(GL_TEXTURE_2D);
 if (item->texture_id == -1) {
   glGenTextures(1, &item->texture_id);
 }
 glBindTexture(GL_TEXTURE_2D, texture_id);
 glXBindTexImageEXT(display, item->glxpixmap, GLX_FRONT_EXT, NULL);
 glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
 glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
 glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
}
