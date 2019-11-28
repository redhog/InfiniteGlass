#include "property_svg.h"
#include "texture.h"
#include "rendering.h"
#include <cairo.h>
#include "glapi.h"
#include "xapi.h"
#include "wm.h"
#include "debug.h"
#include <librsvg/rsvg.h>

typedef struct {
  int x;
  int y;
  int width;
  int height;
  int _width;
  int _height;
  int itemwidth;
  int itemheight;

  RsvgHandle *rsvg;
  RsvgDimensionData dimension;
 
  cairo_surface_t *surface;
  cairo_t *cairo_ctx;
 
  Texture texture;
} SvgPropertyData;

typedef struct { 
  char *transform_str;
  GLint transform_location;
  GLint texture_location;
} SvgPropertyProgramData;

void property_svg_update_drawing(Property *prop, Rendering *rendering) {
  View *view = rendering->view;
  SvgPropertyData *data = (SvgPropertyData *) prop->data;

  float x1, y1, x2, y2;
  int px1, py1, px2, py2;
  int itempixelwidth, itempixelheight;
  int pixelwidth, pixelheight;

  if(!rendering->item->prop_coords) {
    DEBUG("window.svg.error", "Property coords not set\n");   
    return;
  }

  float *coords = (float *) rendering->item->prop_coords->data;
  
  itempixelwidth = coords[2] * view->width / view->screen[2];
  itempixelheight = coords[3] * view->height / view->screen[3];
  
  // x and y are ]0,1[, from top left to bottom right of window.
  x1 = (view->screen[0] - coords[0]) / coords[2];
  y1 = (coords[1] - (view->screen[1] + view->screen[3])) / coords[3];

  x2 = (view->screen[0] + view->screen[2] - coords[0]) / coords[2];
  y2 = (coords[1] - view->screen[1]) / coords[3];
  
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
  
  data->width = pixelwidth;
  data->height = pixelheight;
  data->x = px1;
  data->y = py1;
  data->itemwidth = itempixelwidth;
  data->itemheight = itempixelheight;

  // Actually generate the drawing
  
  RsvgDimensionData dimension;

  rsvg_handle_get_dimensions(data->rsvg, &dimension);

  // Check if current surface size is wrong before recreating...
  if (!data->surface || data->_width != data->width || data->_height != data->height) {
    if (data->cairo_ctx) cairo_destroy(data->cairo_ctx);
    if (data->surface) cairo_surface_destroy(data->surface);
    data->surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, data->width, data->height);
    data->cairo_ctx = cairo_create(data->surface);
    data->_width = data->width;
    data->_height = data->height;
  } else {
    cairo_identity_matrix (data->cairo_ctx);
    cairo_set_operator(data->cairo_ctx, CAIRO_OPERATOR_CLEAR);
    cairo_paint(data->cairo_ctx);
    cairo_set_operator(data->cairo_ctx, CAIRO_OPERATOR_OVER);
  }

  DEBUG("window.svg", "RENDER %d,%d[%f,%f] = [%d,%d]\n",
        -data->x,
        -data->y,
        dimension.em,
        dimension.ex,
        data->itemwidth,
        data->itemheight);
  
  cairo_translate(data->cairo_ctx,
                  -data->x,
                  -data->y);
  cairo_scale(data->cairo_ctx,
              (float) data->itemwidth / (float) dimension.em,
              (float) data->itemheight / (float) dimension.ex);
  
  rsvg_handle_render_cairo(data->rsvg, data->cairo_ctx);
  cairo_surface_flush(data->surface);  
  texture_from_cairo_surface(&data->texture, data->surface);
}


void property_svg_init(PropertyTypeHandler *prop) { prop->type = XInternAtom(display, "IG_SVG", False); prop->name = AnyPropertyType; }
void property_svg_load(Property *prop) {
  SvgPropertyData *data = (SvgPropertyData *) prop->data;
  if (!data) {
    prop->data = malloc(sizeof(SvgPropertyData));
    data = (SvgPropertyData *) prop->data;

    data->surface = NULL;
    data->cairo_ctx = NULL;
    data->rsvg = NULL;

    texture_initialize(&data->texture);
  } else {
    if (data->rsvg) g_object_unref(data->rsvg);
    data->rsvg = NULL;
  }
    
  GError *error = NULL;
  unsigned char *src = (unsigned char *) prop->values.bytes;
  data->rsvg = rsvg_handle_new_from_data(src, strlen((char *) src), &error);
  if (!data->rsvg) {
    DEBUG("window.svg.error", "Unable to load svg: %s: %s, len=%ld\n",  error->message, src, prop->nitems);
    fflush(stdout);
    return;
  }
  rsvg_handle_get_dimensions(data->rsvg, &data->dimension);  
}

void property_svg_free(Property *prop) {
  SvgPropertyData *data = (SvgPropertyData *) prop->data;
  if (!data) return;
  if (data->cairo_ctx) cairo_destroy(data->cairo_ctx);
  if (data->surface) cairo_surface_destroy(data->surface);
  texture_destroy(&data->texture);
  if (data->rsvg) g_object_unref(data->rsvg);
  data->cairo_ctx = NULL;
  data->surface = NULL;
  data->rsvg = NULL;
  free(data);
  prop->data = NULL;
}

void property_svg_to_gl(Property *prop, Rendering *rendering) {
  if (rendering->view->picking) return;
  PropertyProgramCache *prop_cache = &prop->programs[rendering->program_cache_idx];
  SvgPropertyData *data = (SvgPropertyData *) prop->data;
  SvgPropertyProgramData *program_data = (SvgPropertyProgramData *) prop_cache->data;
  
  if (program_data->texture_location == -1 || program_data->transform_location == -1) return;
  if (!data->rsvg) return;
  
  property_svg_update_drawing(prop, rendering);
  
  gl_check_error("property_svg_to_gl1");
  
  float transform[4] = {(float) data->x / (float) data->itemwidth,
                        (float) data->y / (float) data->itemheight,
                        (float) data->width / (float) data->itemwidth,
                        (float) data->height / (float) data->itemheight};
  glUniform4fv(program_data->transform_location, 1, transform);

  glUniform1i(program_data->texture_location, rendering->texture_unit);
  glActiveTexture(GL_TEXTURE0 + rendering->texture_unit);
  glBindTexture(GL_TEXTURE_2D, data->texture.texture_id);
  glBindSampler(rendering->texture_unit, 0);
  rendering->texture_unit++;
  
  gl_check_error("property_svg_to_gl2");
}
void property_svg_print(Property *prop, FILE *fp) {
  fprintf(fp, "%ld.%s=<svg>\n", prop->window, prop->name_str);
}
void property_svg_load_program(Property *prop, Rendering *rendering) {
  PropertyProgramCache *prop_cache = &prop->programs[rendering->program_cache_idx];

  prop_cache->data = malloc(sizeof(SvgPropertyData));
  SvgPropertyProgramData *program_data = (SvgPropertyProgramData *) prop_cache->data;

  program_data->transform_str = malloc(strlen(prop_cache->name_str) + strlen("_transform") + 1);
  strcpy(program_data->transform_str, prop_cache->name_str);
  strcpy(program_data->transform_str + strlen(prop_cache->name_str), "_transform");

  program_data->texture_location = glGetUniformLocation(prop_cache->program, prop_cache->name_str);
  program_data->transform_location = glGetUniformLocation(prop_cache->program, program_data->transform_str);
  char *status = NULL;
  if (program_data->texture_location != -1 && program_data->transform_location != -1) {
    status = "enabled";
  } else if (program_data->transform_location != -1) {
    status = "only transform enabled";
  } else if (program_data->texture_location != -1) {
    status = "only texture enabled";
  } else {
    status = "disabled";
  }
  DEBUG("prop", "%ld[%ld].%s %s (svg) [%d]\n",
        prop->window,
        rendering->shader->program, prop->name_str, status, prop->nitems);
}
void property_svg_free_program(Property *prop, size_t index) {
  PropertyProgramCache *prop_cache = &prop->programs[index];
  if (!prop_cache->data) return;
  SvgPropertyProgramData *program_data = (SvgPropertyProgramData *) prop_cache->data;
  free(program_data->transform_str);
  free(program_data);
  prop_cache->data = NULL;
}
PropertyTypeHandler property_svg = {&property_svg_init, &property_svg_load, &property_svg_free, &property_svg_to_gl, &property_svg_print, &property_svg_load_program, &property_svg_free_program};
