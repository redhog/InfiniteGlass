#include "item_widget.h"
#include "item_widget_shader.h"
#include "texture.h"
#include "glapi.h"
#include "xapi.h"
#include "wm.h"
#include "debug.h"
#include <librsvg/rsvg.h>

void item_type_widget_update_tile(ItemWidget *item, ItemWidgetTile *tile) {
  GError *error;
  RsvgDimensionData dimension;

  RsvgHandle *rsvg = rsvg_handle_new_from_file(item->label, &error);
  if (!rsvg) {
    printf("Unable to load svg: %s\n",  error->message);
    fflush(stdout);
  }
  rsvg_handle_get_dimensions(rsvg, &dimension);

  // Check if current surface size is wrong before recreating...
  if (1 || !tile->surface) {
    if (tile->surface) {
      cairo_surface_destroy(tile->surface);
    }
   
    tile->surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, tile->width, tile->height);
  }
  
  cairo_t *cairo_ctx = cairo_create(tile->surface);

  DEBUG("widget.render",
        "RENDER %d,%d[%d,%d]/ = [%d,%d]\n",
        -tile->x,
        -tile->y,
        dimension.width,
        dimension.height,
        tile->itemwidth, 
        tile->itemheight); 
  
  cairo_translate(cairo_ctx,
                  -tile->x,
                  -tile->y);
  cairo_scale(cairo_ctx,
              (float) tile->itemwidth / (float) dimension.width,
              (float) tile->itemheight / (float) dimension.height);
  
  rsvg_handle_render_cairo(rsvg, cairo_ctx);
  g_object_unref(rsvg);
  cairo_destroy(cairo_ctx);

  cairo_surface_flush(tile->surface);
  
  texture_from_cairo_surface(&tile->texture, tile->surface);
}

ItemWidgetTile *item_type_widget_get_tile(View *view, ItemWidget *item) {
  float x1, y1, x2, y2;
  int px1, py1, px2, py2;
  int itempixelwidth, itempixelheight;
  int pixelwidth, pixelheight;
  ItemWidgetTile *tile;
  
  itempixelwidth = item->base.coords[2] * view->width / view->screen[2];
  itempixelheight = item->base.coords[3] * view->height / view->screen[3];
  
  // x and y are ]0,1[, from top left to bottom right of window.
  x1 = (view->screen[0] - item->base.coords[0]) / item->base.coords[2];
  y1 = (item->base.coords[1] - (view->screen[1] + view->screen[3])) / item->base.coords[3];

  x2 = (view->screen[0] + view->screen[2] - item->base.coords[0]) / item->base.coords[2];
  y2 = (item->base.coords[1] - view->screen[1]) / item->base.coords[3];
  
  if (x1 < 0.) x1 = 0.;
  if (x1 > 1.) x1 = 1.;
  if (y1 < 0.) y1 = 0.;
  if (y1 > 1.) y1 = 1.;
  if (x2 < 0.) x2 = 0.;
  if (x2 > 1.) x2 = 1.;
  if (y2 < 0.) y2 = 0.;
  if (y2 > 1.) y2 = 1.;
  
  // When screen to window is 1:1 this holds:
  // item->coords[2] = item->width * view->screen[2] / view->width;
  // item->coords[3] = item->height * view->screen[3] / view->height;
  
  px1 = (int) (x1 * (float) itempixelwidth);
  px2 = (int) (x2 * (float) itempixelwidth);
  py1 = (int) (y1 * (float) itempixelheight);
  py2 = (int) (y2 * (float) itempixelheight);

  pixelwidth = px2 - px1;
  pixelheight = py2 - py1;
  
  tile = &item->tiles[0];
  tile->width = pixelwidth;
  tile->height = pixelheight;
  tile->x = px1;
  tile->y = py1;
  tile->itemwidth = itempixelwidth;
  tile->itemheight = itempixelheight;

  item_type_widget_update_tile(item, tile);
  
  return tile;
}

void item_type_widget_constructor(Item *item, void *args) {
  ItemWidget *item_widget = (ItemWidget *) item;
  char *label = (char *) args;
 
  item_widget->label = label;
  item_widget->base.coords[0] = .25;
  item_widget->base.coords[1] = .25;
  item_widget->base.coords[2] = .25;
  item_widget->base.coords[3] = .25;
  item_widget->base.width = 200;
  item_widget->base.height = 200;
  item_widget->base.is_mapped = 1;

  for (int i = 0; i < ITEM_WIDGET_TILE_CACHE_SIZE; i++) {
    texture_initialize(&item_widget->tiles[i].texture);
    item_widget->tiles[i].surface = 0;
  }
}

void item_type_widget_destructor(Item *item) {
  ItemWidget *item_widget = (ItemWidget *) item;

  for (int i = 0; i < ITEM_WIDGET_TILE_CACHE_SIZE; i++) {
    if (item_widget->tiles[i].surface) {
      cairo_surface_destroy(item_widget->tiles[i].surface);
    }
    texture_destroy(&item_widget->tiles[i].texture);
  }
}

void item_type_widget_draw(View *view, Item *item) {
  //ItemWidget *item_widget = (ItemWidget *) item;

  ItemWidgetShader *shader = (ItemWidgetShader *) item->type->get_shader(item);

  ItemWidgetTile *tile = item_type_widget_get_tile(view, (ItemWidget *) item);
  
  gl_check_error("item_type_widget_draw1");
  
  float transform[4] = {(float) tile->x / (float) tile->itemwidth,
                        (float) tile->y / (float) tile->itemheight,
                        (float) tile->width / (float) tile->itemwidth,
                        (float) tile->height / (float) tile->itemheight};
  
  DEBUG("widget.draw", "DRAW %f,%f[%f,%f] from tile %f,%f[%f,%f]\n",
         item->coords[0],
         item->coords[1],
         item->coords[2],
         item->coords[3],
         transform[0],
         transform[1],
         transform[2],
         transform[3]);
  
  glUniform4fv(shader->transform_attr, 1, transform);
  
  glUniform1i(shader->texture_attr, 0);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, tile->texture.texture_id);
  glBindSampler(1, 0);
 
  gl_check_error("item_type_widget_draw2");

  item_type_widget.base->draw(view, item);

  gl_check_error("item_type_widget_draw3");
}

void item_type_widget_update(Item *item) { 
  GError *error;
  RsvgDimensionData dimension;

  RsvgHandle *rsvg = rsvg_handle_new_from_file(((ItemWidget *) item)->label, &error);
  rsvg_handle_get_dimensions(rsvg, &dimension);
  g_object_unref(rsvg);

  item->width = dimension.width;
  item->height = dimension.height;
  
  item_type_widget.base->update(item);
}

Shader *item_type_widget_get_shader(Item *item) {
  return (Shader *) item_widget_shader_get();
}

void item_type_widget_print(Item *item) {
  item_type_widget.base->print(item);
  printf("    label=%s\n", ((ItemWidget *) item)->label);
}

ItemType item_type_widget = {
  &item_type_base,
  sizeof(ItemWidget),
  "ItemWidget",
  &item_type_widget_constructor,
  &item_type_widget_destructor,
  &item_type_widget_draw,
  &item_type_widget_update,
  &item_type_widget_get_shader,
  &item_type_widget_print
};

Item *item_get_widget(char *label) {
  return item_create(&item_type_widget, label);
}
