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
   
 if (idx+1 > items_all_size) {
  if (!items_all_size) items_all_size = 8;
  items_all_size *=2;
  items_all = realloc(items_all, sizeof(Item *) * items_all_size);
 }

 item = (Item *) malloc(sizeof(Item));
 item->pixmap = 0;
 item->texture_id = -1;
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

void item_update_space_pos_from_window(Item *item) {
  XWindowAttributes attr;
  XGetWindowAttributes(display, item->window, &attr);

  int x                     = attr.x;
  int y                     = attr.y;
  int width                 = attr.width;
  int height                = attr.height;
  float left = ((float) (x - overlay_attr.x)) / (float) overlay_attr.width;
  float right = left + (float) width / (float) overlay_attr.width;
  float top = ((float) y - overlay_attr.y) / (float) overlay_attr.height;
  float bottom = top + (float) height / (float) overlay_attr.height;

  left = 2. * left - 1.; 
  right = 2. * right - 1.; 
  top = 1 - 2. * top; 
  bottom = 1. - 2. * bottom; 

  fprintf(stderr, "%u: %i,%i(%i,%i)\n", (uint) item->window, x, y, width, height); fflush(stderr);
    
  item->space_pos[0][0] = left; item->space_pos[0][1] = bottom;
  item->space_pos[1][0] = left; item->space_pos[1][1] = top;
  item->space_pos[2][0] = right; item->space_pos[2][1] = bottom;
  item->space_pos[3][0] = right; item->space_pos[3][1] = top;

  item_update_space_pos(item);
}

void item_update_space_pos(Item *item) {  
  glGenBuffers(1, &item->space_pos_vbo); 
  glBindBuffer(GL_ARRAY_BUFFER, item->space_pos_vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(item->space_pos), item->space_pos, GL_STATIC_DRAW);
}

void item_update_texture(Item *item) {
 if (item->pixmap) XFreePixmap(display, item->pixmap);
 // FIXME: free all other stuff if already created
 
 item->pixmap = XCompositeNameWindowPixmap(display, item->window);
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
 glBindTexture(GL_TEXTURE_2D, item->texture_id);
 glXBindTexImageEXT(display, item->glxpixmap, GLX_FRONT_EXT, NULL);
 glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
 glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
 glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
}
