#include "glapi.h"
#include "item_window_pixmap.h"
#include "item_window_shader.h"
#include "xapi.h"
#include "wm.h"

void item_type_window_pixmap_constructor(Item *item, void *args) {
  ItemWindowPixmap *pixmap_item = (ItemWindowPixmap *) item;
  ItemWindow *window_item = (ItemWindow *) item;

  item_type_window.init(item, args);
  
  pixmap_item->window_pixmap = 0;
  texture_initialize(&pixmap_item->window_texture);
  texture_initialize(&pixmap_item->icon_texture);
  texture_initialize(&pixmap_item->icon_mask_texture);

  pixmap_item->damage = XDamageCreate(display, window_item->window, XDamageReportNonEmpty);
  pixmap_item->wm_hints.flags = 0;

  XErrorEvent error;
  x_try();
  XWMHints *wm_hints = XGetWMHints(display, window_item->window);
  if (wm_hints) {
    pixmap_item->wm_hints = *wm_hints;
    XFree(wm_hints);
  }
  if (!x_catch(&error)) {
    printf("Window does not have any WM_HINTS: %lu", window_item->window);
  }
}

void item_type_window_pixmap_destructor(Item *item) {
  ItemWindowPixmap *pixmap_item = (ItemWindowPixmap *) item;
  texture_destroy(&pixmap_item->window_texture);
  texture_destroy(&pixmap_item->icon_texture);
  texture_destroy(&pixmap_item->icon_mask_texture);
  item_type_window.destroy(item);
}

void item_type_window_pixmap_draw(View *view, Item *item) {
  /*
  printf("XXXXXXXXXXXXXXXXX\n");
  item_type_base_print(item);
  printf("\n\n");
  */
 
  if (item->is_mapped) {
    ItemWindowPixmap *pixmap_item = (ItemWindowPixmap *) item;

    ItemWindowShader *shader = (ItemWindowShader *) item->type->get_shader(item);
    
    texture_from_pixmap(&pixmap_item->window_texture, pixmap_item->window_pixmap);

    glUniform1i(shader->window_sampler_attr, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, pixmap_item->window_texture.texture_id);
    glBindSampler(1, 0);
    
    if (pixmap_item->wm_hints.flags & IconPixmapHint) {
      glUniform1i(shader->has_icon_attr, 1);
      glUniform1i(shader->icon_sampler_attr, 1);
      glActiveTexture(GL_TEXTURE1);
      glBindTexture(GL_TEXTURE_2D, pixmap_item->icon_texture.texture_id);
      glBindSampler(1, 0);
    } else {
      glUniform1i(shader->has_icon_attr, 0);
    }
    if (pixmap_item->wm_hints.flags & IconMaskHint) {
      glUniform1i(shader->has_icon_mask_attr, 1);
      glUniform1i(shader->icon_mask_sampler_attr, 2);
      glActiveTexture(GL_TEXTURE2);
      glBindTexture(GL_TEXTURE_2D, pixmap_item->icon_mask_texture.texture_id);
      glBindSampler(1, 0);
    } else {
      glUniform1i(shader->has_icon_mask_attr, 1);
    }
    
    item_type_window.draw(view, item);
  }
}

void item_type_window_pixmap_update(Item *item) {
  ItemWindowPixmap *pixmap_item = (ItemWindowPixmap *) item;
  ItemWindow *window_item = (ItemWindow *) item;
   
  if (!item->is_mapped) return;

  item_type_window.update(item);

  x_push_error_context("item_update_pixmap");
  
  // FIXME: free all other stuff if already created

  pixmap_item->window_pixmap = XCompositeNameWindowPixmap(display, window_item->window);

  Window root_return;
  int x_return;
  int y_return;
  unsigned int width_return;
  unsigned int height_return;
  unsigned int border_width_return;
  unsigned int depth_return;
  
  Status res = XGetGeometry(display, pixmap_item->window_pixmap, &root_return, &x_return, &y_return, &width_return, &height_return, &border_width_return, &depth_return);
  /*
  printf("XGetGeometry(pixmap=%lu) = status=%d, root=%lu, x=%u, y=%u, w=%u, h=%u border=%u depth=%u\n",
         pixmap_item->window_pixmap, res, root_return, x_return, y_return, width_return, height_return, border_width_return, depth_return);
  */
  
  texture_from_pixmap(&pixmap_item->window_texture, pixmap_item->window_pixmap);

  if (pixmap_item->wm_hints.flags & IconPixmapHint) {
    texture_from_pixmap(&pixmap_item->icon_texture, pixmap_item->wm_hints.icon_pixmap);
  }
  if (pixmap_item->wm_hints.flags & IconMaskHint) {
    texture_from_pixmap(&pixmap_item->icon_mask_texture, pixmap_item->wm_hints.icon_mask);
  }
  gl_check_error("item_update_pixmap2");

  x_pop_error_context();
}

Shader *item_type_window_pixmap_get_shader(Item *item) {
  return (Shader *) item_window_shader_get();
}

ItemType item_type_window_pixmap = {
  &item_type_window,
  sizeof(ItemWindowPixmap),
  &item_type_window_pixmap_constructor,
  &item_type_window_pixmap_destructor,
  &item_type_window_pixmap_draw,
  &item_type_window_pixmap_update,
  &item_type_window_pixmap_get_shader
};
