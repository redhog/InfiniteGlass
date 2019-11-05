#include "item_window_svg.h"
#include "item_window_svg_shader.h"
#include "texture.h"
#include "glapi.h"
#include "xapi.h"
#include "wm.h"
#include "debug.h"
#include <librsvg/rsvg.h>

void item_type_window_svg_update_drawing(View *view, ItemWindowSVG *item) {
  float x1, y1, x2, y2;
  int px1, py1, px2, py2;
  int itempixelwidth, itempixelheight;
  int pixelwidth, pixelheight;
   
  itempixelwidth = item->base.base.coords[2] * view->width / view->screen[2];
  itempixelheight = item->base.base.coords[3] * view->height / view->screen[3];
  
  // x and y are ]0,1[, from top left to bottom right of window.
  x1 = (view->screen[0] - item->base.base.coords[0]) / item->base.base.coords[2];
  y1 = (item->base.base.coords[1] - (view->screen[1] + view->screen[3])) / item->base.base.coords[3];

  x2 = (view->screen[0] + view->screen[2] - item->base.base.coords[0]) / item->base.base.coords[2];
  y2 = (item->base.base.coords[1] - view->screen[1]) / item->base.base.coords[3];
  
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
  
  item->drawing.width = pixelwidth;
  item->drawing.height = pixelheight;
  item->drawing.x = px1;
  item->drawing.y = py1;
  item->drawing.itemwidth = itempixelwidth;
  item->drawing.itemheight = itempixelheight;

  // Actually generate the drawing
  
  GError *error;
  RsvgDimensionData dimension;

  RsvgHandle *rsvg = rsvg_handle_new_from_data((unsigned char *) item->source, strlen(item->source), &error);
  if (!rsvg) {
    DEBUG("window.svg.error", "Unable to load svg: %s\n",  error->message);
    fflush(stdout);
  }
  rsvg_handle_get_dimensions(rsvg, &dimension);

  // Check if current surface size is wrong before recreating...
  if (1 || !item->drawing.surface) {
    if (item->drawing.surface) {
      cairo_surface_destroy(item->drawing.surface);
    }
   
    item->drawing.surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, item->drawing.width, item->drawing.height);
  }
  
  cairo_t *cairo_ctx = cairo_create(item->drawing.surface);

  DEBUG("window.svg", "RENDER %d,%d[%d,%d] = [%d,%d]\n",
        -item->drawing.x,
        -item->drawing.y,
        dimension.width,
        dimension.height,
        item->drawing.itemwidth,
        item->drawing.itemheight);
  
  cairo_translate(cairo_ctx,
                  -item->drawing.x,
                  -item->drawing.y);
  cairo_scale(cairo_ctx,
              (float) item->drawing.itemwidth / (float) dimension.width,
              (float) item->drawing.itemheight / (float) dimension.height);
  
  rsvg_handle_render_cairo(rsvg, cairo_ctx);
  g_object_unref(rsvg);
  cairo_destroy(cairo_ctx);

  cairo_surface_flush(item->drawing.surface);
  
  texture_from_cairo_surface(&item->drawing.texture, item->drawing.surface);
}

void item_type_window_svg_constructor(Item *item, void *args) {
  ItemWindowSVG *item_window_svg = (ItemWindowSVG *) item;
  ItemWindowSVGArgs *svg_args = (ItemWindowSVGArgs *) args;
 
  item_window_svg->source = svg_args->source;

  texture_initialize(&item_window_svg->drawing.texture);
  item_window_svg->drawing.surface = 0;

  item_type_window.init(item, &svg_args->window);
}

void item_type_window_svg_destructor(Item *item) {
  ItemWindowSVG *item_window_svg = (ItemWindowSVG *) item;

  if (item_window_svg->drawing.surface) {
    cairo_surface_destroy(item_window_svg->drawing.surface);
  }
  texture_destroy(&item_window_svg->drawing.texture);
}

void item_type_window_svg_draw(View *view, Item *item) {
  ItemWindowSVG *item_window_svg = (ItemWindowSVG *) item;

  ItemWindowSvgShader *shader = (ItemWindowSvgShader *) item->type->get_shader(item);

  item_type_window_svg_update_drawing(view, item_window_svg);
  
  gl_check_error("item_type_window_svg_draw1");
  
  float transform[4] = {(float) item_window_svg->drawing.x / (float) item_window_svg->drawing.itemwidth,
                        (float) item_window_svg->drawing.y / (float) item_window_svg->drawing.itemheight,
                        (float) item_window_svg->drawing.width / (float) item_window_svg->drawing.itemwidth,
                        (float) item_window_svg->drawing.height / (float) item_window_svg->drawing.itemheight};
  
  DEBUG("window.svg.draw",
         "DRAW %f,%f[%f,%f] from tile %f,%f[%f,%f]\n",
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
  glBindTexture(GL_TEXTURE_2D, item_window_svg->drawing.texture.texture_id);
  glBindSampler(1, 0);
 
  gl_check_error("item_type_window_svg_draw2");

  item_type_window_svg.base->draw(view, item);

  gl_check_error("item_type_window_svg_draw3");
}

void item_type_window_svg_update(Item *item) { 
  GError *error;
  RsvgDimensionData dimension;

  RsvgHandle *rsvg = rsvg_handle_new_from_data((unsigned char *) ((ItemWindowSVG *) item)->source, strlen(((ItemWindowSVG *) item)->source), &error);
  rsvg_handle_get_dimensions(rsvg, &dimension);
  g_object_unref(rsvg);

  int arr[2] = {dimension.width, dimension.height};
  XChangeProperty(display, item->window, IG_SIZE, XA_INTEGER, 32, PropModeReplace, (void *) arr, 2);
  
  item_type_window_svg.base->update(item);
}

Shader *item_type_window_svg_get_shader(Item *item) {
  return (Shader *) item_window_svg_shader_get();
}

void item_type_window_svg_print(Item *item) {
  item_type_window_svg.base->print(item);
}


ItemType item_type_window_svg = {
  &item_type_window,
  sizeof(ItemWindowSVG),
  "ItemWindowSVG",
  &item_type_window_svg_constructor,
  &item_type_window_svg_destructor,
  &item_type_window_svg_draw,
  &item_type_window_svg_update,
  &item_type_window_svg_get_shader,
  &item_type_window_svg_print
};
