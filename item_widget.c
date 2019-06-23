#include "item_widget.h"
#include "item_window_shader.h"
#include "texture.h"
#include "glapi.h"
#include "xapi.h"
#include "wm.h"
#include <librsvg/rsvg.h>
#include <librsvg/rsvg-cairo.h>

void item_type_widget_destructor(Item *item) {
  WidgetItem *widget_item = (WidgetItem *) item;

  if (widget_item->surface) {
    cairo_surface_destroy(widget_item->surface);
  }
  
  texture_destroy(&widget_item->texture);
}

void item_type_widget_draw(View *view, Item *item) {
  WidgetItem *widget_item = (WidgetItem *) item;

  ItemWindowShader *shader = (ItemWindowShader *) item->type->get_shader(item);
  
  gl_check_error("item_type_widget_draw1");
  
  glUniform1i(shader->window_sampler_attr, 0);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, widget_item->texture.texture_id);
  glBindSampler(1, 0);
 
  gl_check_error("item_type_widget_draw2");

  item_type_base.draw(view, item);

  gl_check_error("item_type_widget_draw3");
}

void item_type_widget_update(Item *item) {
  WidgetItem *widget_item = (WidgetItem *) item;
  GError *error;
  RsvgDimensionData dimension;

  RsvgHandle *rsvg = rsvg_handle_new_from_file(widget_item->label, &error);
  rsvg_handle_get_dimensions(rsvg, &dimension);

  if (!widget_item->surface || dimension.width != item->width || dimension.height != item->height) {
    if (widget_item->surface) {
      cairo_surface_destroy(widget_item->surface);
    }
   
    widget_item->surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, dimension.width, dimension.height);
    item->width = dimension.width;
    item->height = dimension.height;
  }
  
  cairo_t *cairo_ctx = cairo_create(widget_item->surface);


  cairo_translate(cairo_ctx, -100, -100);
  //cairo_scale(cairo_ctx, 2, 2);
  
  rsvg_handle_render_cairo(rsvg, cairo_ctx);
  g_object_unref(rsvg);
  cairo_destroy(cairo_ctx);

  cairo_surface_flush(widget_item->surface);
  
  texture_initialize(&widget_item->texture);
  if (!widget_item->texture.texture_id) {
    glGenTextures(1, &widget_item->texture.texture_id);
  }
  
  glBindTexture(GL_TEXTURE_2D, widget_item->texture.texture_id);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_BLUE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_GREEN);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_RED);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_ALPHA);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, item->width, item->height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
               cairo_image_surface_get_data(widget_item->surface));
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  gl_check_error("texture_from_pixmap4");
 
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
  item->surface = NULL;
  item->base.coords[0] = .25;
  item->base.coords[1] = .25;
  item->base.coords[2] = .25;
  item->base.coords[3] = .25;
  item->base.width = 0;
  item->base.height = 0;
  item->base.type = &item_type_widget;
  item_add((Item *) item);
  item->base.is_mapped = 1;

  item_type_widget.update((Item*) item);

  return item;
}
