#include "item_widget.h"
#include "item_window_shader.h"
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

  if (1 || !tile->surface || dimension.width != item->base.width || dimension.height != item->base.height) {
    if (tile->surface) {
      cairo_surface_destroy(tile->surface);
    }
   
    tile->surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, dimension.width, dimension.height);
    item->base.width = dimension.width;
    item->base.height = dimension.height;
    printf("XXXXXXXXXXXXXXXX %d, %d\n",     dimension.width, dimension.height);
  }
  
  cairo_t *cairo_ctx = cairo_create(tile->surface);

  
  cairo_translate(cairo_ctx, -tile->x, -tile->y);
  cairo_scale(cairo_ctx, tile->zoom, tile->zoom);
  
  rsvg_handle_render_cairo(rsvg, cairo_ctx);
  g_object_unref(rsvg);
  cairo_destroy(cairo_ctx);

  cairo_surface_flush(tile->surface);
  
  texture_from_cairo_surface(&tile->texture, tile->surface);
}

WidgetItemTile *item_type_widget_get_tile(View *view, WidgetItem *item) {
  float x, y, zoom;

  // Screen to window 1:1
  // item->coords[2] = item->width * view->screen[2] / view->width;
  // item->coords[3] = item->height * view->screen[3] / view->height;
  // -> zoom = 1
  
  zoom = ((float) item->base.width / (float) view->width) * (view->screen[2] / item->base.coords[2]);
  
  x = item->base.width * (item->base.coords[0] - view->screen[0]) / item->base.coords[2];
  y = item->base.height * (1 - (item->base.coords[1] - view->screen[1]) / item->base.coords[3]);

  item->tiles[0].x = x + zoom * 10;
  item->tiles[0].y = y;
  item->tiles[0].zoom = zoom;
  item_type_widget_update_tile(item, &item->tiles[0]);

  printf("Update tile %f, %f, %f\n", x, y, zoom);
  return &item->tiles[0];
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

  ItemWindowShader *shader = (ItemWindowShader *) item->type->get_shader(item);

  WidgetItemTile *tile = item_type_widget_get_tile(view, (WidgetItem *) item);
  
  gl_check_error("item_type_widget_draw1");
  
  glUniform1i(shader->window_sampler_attr, 0);
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

  /*
  item->width = dimension.width;
  item->height = dimension.height;
  */
  item->width = default_view.width;
  item->height = default_view.height;

  
  item_type_base.update(item);
}

Shader *item_type_widget_get_shader(Item *item) {
  return (Shader *) item_window_shader_get();
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
