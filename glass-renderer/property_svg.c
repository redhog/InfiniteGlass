#include "property_svg.h"
#include "svg_templating.h"
#include "property_coords.h"
#include "texture.h"
#include "rendering.h"
#include <cairo.h>
#include "glapi.h"
#include "xapi.h"
#include "wm.h"
#include "debug.h"
#include <librsvg/rsvg.h>

#include <unistd.h>
#include <fcntl.h>

typedef struct {
  int x;
  int y;
  int width;
  int height;
  int itemwidth;
  int itemheight;

  SvgTemplating *template;
  char *templated_source;
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
  if (!data->rsvg) {
    DEBUG("window.svg.error", "prop->rsvg = NULL: Calculate not called before to_gl.\n");
    return;
  }

  float *coords = ((PropertyCoords *) rendering->item->prop_coords->data)->ccoords;
  
  itempixelwidth = coords[2] * view->width / view->screen[2];
  itempixelheight = coords[3] * view->height / view->screen[3];

  if (itempixelwidth <= 0 || itempixelheight <= 0) {
    DEBUG("window.svg.error", "Property coords width/height zero\n");
    return;
  }
  
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

  if (pixelwidth <= 0 || pixelheight <= 0) {
    DEBUG("window.svg.error", "Selected plot area is zero\n");
    return;
  }

  if (   (data->width == pixelwidth)
      && (data->height == pixelheight)
      && (data->x = px1)
      && (data->y = py1)
      && (data->itemwidth = itempixelwidth)
      && (data->itemheight = itempixelheight))
    return; 

  // Actually generate the drawing
  
  RsvgDimensionData dimension;

  // Shortcut if we're just rerendering the same stuff...
  if (data->surface
      && data->width == pixelwidth
      && data->height == pixelheight
      && data->x == px1
      && data->y == py1
      && data->itemwidth == itempixelwidth
      && data->itemheight == itempixelheight) return;
  
  // Check if current surface size is wrong before recreating...
  if (!data->surface || pixelwidth != data->width || pixelheight != data->height) {
    if (data->cairo_ctx) cairo_destroy(data->cairo_ctx);
    if (data->surface) cairo_surface_destroy(data->surface);
    data->surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, pixelwidth, pixelheight);
    data->cairo_ctx = cairo_create(data->surface);
  } else {
    cairo_identity_matrix (data->cairo_ctx);
    cairo_set_operator(data->cairo_ctx, CAIRO_OPERATOR_CLEAR);
    cairo_paint(data->cairo_ctx);
    cairo_set_operator(data->cairo_ctx, CAIRO_OPERATOR_OVER);
  }

  data->width = pixelwidth;
  data->height = pixelheight;
  data->x = px1;
  data->y = py1;
  data->itemwidth = itempixelwidth;
  data->itemheight = itempixelheight;

  rsvg_handle_get_dimensions(data->rsvg, &dimension);
  
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

  float xscale = data->itemwidth / (float) dimension.em;
  float yscale = (float) data->itemheight / (float) dimension.ex;
  float scale = xscale < yscale ? xscale : yscale;

  cairo_scale(data->cairo_ctx, scale, scale);

  if (cairo_status(data->cairo_ctx) == CAIRO_STATUS_SUCCESS) {  
    rsvg_handle_render_cairo(data->rsvg, data->cairo_ctx);
    cairo_surface_flush(data->surface);  
    texture_from_cairo_surface(&data->texture, data->surface);
  } else {
    cairo_destroy(data->cairo_ctx);
    cairo_surface_destroy(data->surface);
    data->cairo_ctx = NULL;
    data->surface = NULL;
  }
}


void property_svg_init(PropertyTypeHandler *prop) { prop->type = ATOM("IG_SVG"); prop->name = AnyPropertyType; }
void property_svg_load(Property *prop) {
  SvgPropertyData *data = (SvgPropertyData *) prop->data;
  if (!data) {
    prop->data = malloc(sizeof(SvgPropertyData));
    data = (SvgPropertyData *) prop->data;

    data->template = NULL;
    data->templated_source = NULL;
    data->surface = NULL;
    data->cairo_ctx = NULL;
    data->rsvg = NULL;

    texture_initialize(&data->texture);
  }

  if (data->template) svg_templating_free(data->template);
  data->template = svg_templating_create(prop->values.bytes);
}

void property_svg_free(Property *prop) {
  SvgPropertyData *data = (SvgPropertyData *) prop->data;
  if (!data) return;
  if (data->template) svg_templating_free(data->template);
  if (data->templated_source) free(data->templated_source);
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
  if (rendering->picking) return;
  PropertyProgramCache *prop_cache = &prop->programs[rendering->program_cache_idx];
  SvgPropertyData *data = (SvgPropertyData *) prop->data;
  SvgPropertyProgramData *program_data = (SvgPropertyProgramData *) prop_cache->data;
  
  if (!prop_cache->is_uniform || program_data->texture_location == -1 || program_data->transform_location == -1) return;
  if (!data->rsvg) return;
  
  glActiveTexture(GL_TEXTURE0 + rendering->texture_unit);

  property_svg_update_drawing(prop, rendering);
  
  GL_CHECK_ERROR("property_svg_to_gl1", "%ld", prop->window);
  
  float transform[4] = {(float) data->x / (float) data->itemwidth,
                        (float) data->y / (float) data->itemheight,
                        (float) data->width / (float) data->itemwidth,
                        (float) data->height / (float) data->itemheight};
  glUniform4fv(program_data->transform_location, 1, transform);

  glUniform1i(program_data->texture_location, rendering->texture_unit);
  glBindTexture(GL_TEXTURE_2D, data->texture.texture_id);
  glBindSampler(rendering->texture_unit, 0);
  rendering->texture_unit++;

  if (rendering->print) {
    printf("%s%s: !svg [%f, %f, %f, %f]\n", get_indent(rendering->indent), prop->name_str, transform[0], transform[1], transform[2], transform[3]);
  }
  
  GL_CHECK_ERROR("property_svg_to_gl2", "%ld", prop->window);
}
void property_svg_print(Property *prop, int indent, FILE *fp, int detail) {
  fprintf(fp, "%s%s: !SVG\n", get_indent(indent), prop->name_str);
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

uint64_t property_svg_calculate(Property *prop, Rendering *rendering) {
  if (!prop || !prop->data) return prop->version;  
  SvgPropertyData *data = (SvgPropertyData *) prop->data;
  if (!data || !data->template) return prop->version;
  uint64_t required_version = 0;

  if (data->templated_source) free(data->templated_source);

  for (size_t binding_idx = 0; binding_idx < data->template->bindings_count; binding_idx++) {
    BindingEntry *entry = &data->template->bindings[binding_idx];
    if (entry->data) {
      Property *entryprop = (Property *) entry->data;
      if (entryprop->version > prop->calculated_version) {
        svg_templating_replace_by_data(data->template, (void *) entryprop, entryprop->values.bytes);
      }
      required_version = MAX(required_version, entryprop->version);
    } else {
      if (strncmp(entry->url, "property://", 8) != 0) continue;
      for (size_t pathprop_idx = 0; pathprop_idx < rendering->item->properties->properties->count; pathprop_idx++) {
        Property *pathprop = (Property *) rendering->item->properties->properties->entries[pathprop_idx];
        if (strcmp(entry->url + 8, pathprop->name_str) == 0) {
          svg_templating_replace_by_index(data->template, binding_idx, pathprop->values.bytes, (void *) pathprop);
          required_version = MAX(required_version, pathprop->version);
          break;
        }
      }
    }
  }
  svg_templating_gc(data->template);
  data->templated_source = svg_templating_render(data->template);

  GError *error = NULL;
  if (data->rsvg) g_object_unref(data->rsvg);
  data->rsvg = rsvg_handle_new_from_data((unsigned char *) data->templated_source, strlen(data->templated_source), &error);
  if (!data->rsvg) {
    DEBUG("window.svg.error", "Unable to load svg: %s: %s, len=%ld\n",
          error->message, (unsigned char *) data->templated_source, strlen(data->templated_source));
    fflush(stdout);
    return required_version;
  }
  rsvg_handle_get_dimensions(data->rsvg, &data->dimension);  
  
  return required_version;
}

PropertyTypeHandler property_svg = {
  .init=&property_svg_init,
  .load=&property_svg_load,
  .free=&property_svg_free,
  .to_gl=&property_svg_to_gl,
  .print=&property_svg_print,
  .load_program=&property_svg_load_program,
  .free_program=&property_svg_free_program,
  .calculate=&property_svg_calculate
};
