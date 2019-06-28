#include "item_widget.h"
#include "item_widget_shader.h"
#include "texture.h"
#include "glapi.h"
#include "xapi.h"
#include "wm.h"
#include <librsvg/rsvg.h>

void item_type_widget_update_tile(WidgetItem *item, WidgetItemTile *tile) {
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
   
    tile->surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, tile->pixelwidth, tile->pixelheight);
  }
  
  cairo_t *cairo_ctx = cairo_create(tile->surface);

  printf("RENDER %d,%d [%f,%f]\n",
         (int) (-tile->x * tile->itempixelwidth),
         (int) (-tile->y * tile->itempixelheight),
         (float) tile->itempixelwidth / (float) dimension.width,
         (float) tile->itempixelheight / (float) dimension.height);

  cairo_scale(cairo_ctx,
              (float) tile->itempixelwidth / (float) dimension.width,
              (float) tile->itempixelheight / (float) dimension.height);
  cairo_translate(cairo_ctx,
                  (int) (-tile->x * tile->itempixelwidth),
                  (int) (-tile->y * tile->itempixelheight));
  
  rsvg_handle_render_cairo(rsvg, cairo_ctx);
  g_object_unref(rsvg);
  cairo_destroy(cairo_ctx);

  cairo_surface_flush(tile->surface);
  
  texture_from_cairo_surface(&tile->texture, tile->surface);
}

WidgetItemTile *item_type_widget_get_tile(View *view, WidgetItem *item) {
 float x1, y1, x2, y2, width, height;
  int itempixelwidth, itempixelheight;
  int pixelwidth, pixelheight;
  WidgetItemTile *tile;
  
  // x and y are ]0,1[, from top left to bottom right of window.
  x1 = (view->screen[0] - item->base.coords[0]) / item->base.coords[2];
  y1 = (item->base.coords[1] - (view->screen[1] + view->screen[3])) / item->base.coords[3];

  x2 = (view->screen[0] + view->screen[2] - item->base.coords[0]) / item->base.coords[2];
  y2 = (item->base.coords[1] - view->screen[1]) / item->base.coords[3];
  
  if (x1 < 0.) x1 = 0.; if (x1 > 1.) x1 = 1.;
  if (y1 < 0.) y1 = 0.; if (y1 > 1.) y1 = 1.;
  if (x2 < 0.) x2 = 0.; if (x2 > 1.) x2 = 1.;
  if (y2 < 0.) y2 = 0.; if (y2 > 1.) y2 = 1.;

  width = x2 - x1;
  height = y2 - y1;
  
  // When screen to window is 1:1 this holds:
  // item->coords[2] = item->width * view->screen[2] / view->width;
  // item->coords[3] = item->height * view->screen[3] / view->height;

  itempixelwidth = item->base.coords[2] * view->width / view->screen[2];
  itempixelheight = item->base.coords[3] * view->height / view->screen[3];

  pixelwidth = (int) (width * (float) itempixelwidth);
  pixelheight = (int) (height * (float) itempixelheight);

  printf("TILE %f,%f-%f,%f[%f,%f] -> [%d,%d] out of [%d,%d]\n",
         x1, y1,
         x2, y2,
         width, height,
         pixelwidth, pixelheight,
         itempixelwidth, itempixelheight);
   
  tile = &item->tiles[0];
  tile->width = width;
  tile->height = height;
  tile->x = x1;
  tile->y = y1;
  tile->pixelwidth = pixelwidth;
  tile->pixelheight = pixelheight;
  tile->itempixelwidth = itempixelwidth;
  tile->itempixelheight = itempixelheight;

  item_type_widget_update_tile(item, tile);
  
  return tile;
}

void item_type_widget_destructor(Item *item) {
  WidgetItem *widget_item = (WidgetItem *) item;

  for (int i = 0; i < WIDGET_ITEM_TILE_CACHE_SIZE; i++) {
    if (widget_item->tiles[i].surface) {
      cairo_surface_destroy(widget_item->tiles[i].surface);
    }
    texture_destroy(&widget_item->tiles[i].texture);
  }
}

void item_type_widget_draw(View *view, Item *item) {
  WidgetItem *widget_item = (WidgetItem *) item;

  ItemWidgetShader *shader = (ItemWidgetShader *) item->type->get_shader(item);

  WidgetItemTile *tile = item_type_widget_get_tile(view, (WidgetItem *) item);
  
  gl_check_error("item_type_widget_draw1");
  
  float transform[4] = {tile->x * tile->width,
                        tile->y * tile->height,
                        tile->width,
                        tile->height};
  
//  float transform[4] = {0., 0., 1., 1.};
  
  printf("DRAW %f,%f[%f,%f] from tile %f,%f[%f,%f]\n",
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

  item_type_base.draw(view, item);

  gl_check_error("item_type_widget_draw3");
}

void item_type_widget_update(Item *item) { 
  GError *error;
  RsvgDimensionData dimension;

  RsvgHandle *rsvg = rsvg_handle_new_from_file(((WidgetItem *) item)->label, &error);
  rsvg_handle_get_dimensions(rsvg, &dimension);
  g_object_unref(rsvg);

  item->width = dimension.width;
  item->height = dimension.height;
  
  item_type_base.update(item);
}

Shader *item_type_widget_get_shader(Item *item) {
  return (Shader *) item_widget_shader_get();
}

ItemType item_type_widget = {
  &item_type_widget_destructor,
  &item_type_widget_draw,
  &item_type_widget_update,
  &item_type_widget_get_shader
};

Item *item_get_widget(char *label) {
  WidgetItem *item;
  
  item = (WidgetItem *) malloc(sizeof(WidgetItem));
  item->label = label;
  item->base.coords[0] = .25;
  item->base.coords[1] = .25;
  item->base.coords[2] = .25;
  item->base.coords[3] = .25;
  item->base.width = 200;
  item->base.height = 200;
  item->base.type = &item_type_widget;
  item_add((Item *) item);
  item->base.is_mapped = 1;

  for (int i = 0; i < WIDGET_ITEM_TILE_CACHE_SIZE; i++) {
    texture_initialize(&item->tiles[i].texture);
    item->tiles[i].surface = 0;
  }
  
  item_type_widget.update((Item*) item);

  return (Item *) item;
}
