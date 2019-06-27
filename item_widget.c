#include "item_widget.h"
#include "item_widget_shader.h"
#include "texture.h"
#include "glapi.h"
#include "xapi.h"
#include "wm.h"
#include <librsvg/rsvg.h>

void item_type_widget_update_tile(WidgetItem *item, WidgetItemTile *tile) {
  GError *error;

  RsvgHandle *rsvg = rsvg_handle_new_from_file(item->label, &error);
  if (!rsvg) {
    printf("Unable to load svg: %s\n",  error->message);
    fflush(stdout);
  }

  // Check if current surface size is wrong before recreating...
  if (1 || !tile->surface) {
    if (tile->surface) {
      cairo_surface_destroy(tile->surface);
    }
   
    tile->surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, tile->width, tile->height);
  }
  
  cairo_t *cairo_ctx = cairo_create(tile->surface);

  // Bug: scale first, then translate, or we round off to whole pixels at zoom = 1!
  cairo_translate(cairo_ctx, -tile->coords[0] * item->base.width, -tile->coords[1] * item->base.height);
  cairo_scale(cairo_ctx, 1./tile->coords[2], 1./tile->coords[3]);
  
  rsvg_handle_render_cairo(rsvg, cairo_ctx);
  g_object_unref(rsvg);
  cairo_destroy(cairo_ctx);

  cairo_surface_flush(tile->surface);
  
  texture_from_cairo_surface(&tile->texture, tile->surface);
}

WidgetItemTile *item_type_widget_get_tile(View *view, WidgetItem *item) {
  float x, y, zoomx, zoomy;
  int width, height;
  WidgetItemTile *tile;
  
  // Screen to window 1:1
  // item->coords[2] = item->width * view->screen[2] / view->width;
  // item->coords[3] = item->height * view->screen[3] / view->height;
  // -> zoom = 1
  
  zoomx = ((float) item->base.width / (float) view->width) * (view->screen[2] / item->base.coords[2]);
  zoomy = ((float) item->base.height / (float) view->height) * (view->screen[3] / item->base.coords[3]);
  
  // x and y are ]0,1[, from top left to bottom right of window.
  x = (view->screen[0] - item->base.coords[0]) / item->base.coords[2];
  y = (item->base.coords[1] - (view->screen[1] + view->screen[3])) / item->base.coords[3];

  if (x < 0.) x = 0.;
  if (y < 0.) y = 0.;

  width = (int) ((1. - x) * (float) item->base.width * (1./zoomx));
  height = (int) ((1. - y) * (float) item->base.height * (1./zoomy));
  if (width > default_view.width) width = default_view.width;
  if (height > default_view.height) height = default_view.height;

  printf("TILE %f,%f[%d,%d] @ %f,%f ... %f,%f\n",
         x, y, width, height,
         zoomx, zoomy,
         zoomx * (float) width / (float) item->base.width,
         zoomy * (float) height / (float) item->base.height);
   
  tile = &item->tiles[0];
  tile->width = width;
  tile->height = height;
  tile->coords[0] = x;
  tile->coords[1] = y;
  tile->coords[2] = zoomx;
  tile->coords[3] = zoomy;
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

  float transform[4] = {tile->coords[0] * (float) tile->width / (float) item->width,
                        tile->coords[1] * (float) tile->height / (float) item->height,
                        tile->coords[2] * (float) tile->width / (float) item->width,
                        tile->coords[3] * (float) tile->height / (float) item->height};
   
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

  return item;
}
