#include "item_widget.h"
#include "item_window_shader.h"
#include "texture.h"
#include "glapi.h"
#include "xapi.h"
#include "wm.h"

void item_type_widget_destructor(Item *item) {
  WidgetItem *widget_item = (WidgetItem *) item;

  if (widget_item->image) {
    imlib_context_set_image(widget_item->image);
    imlib_free_image();
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
  Imlib_Font font;

  gl_check_error("item_type_widget_update1");

  imlib_context_set_display(display);
  imlib_context_set_visual(DefaultVisual(display, DefaultScreen(display)));
  imlib_context_set_colormap(DefaultColormap(display, DefaultScreen(display)));

  /*
  Imlib_Image image = imlib_load_image("So_happy_smiling_cat.jpg");
  imlib_context_set_image(image);
  width = imlib_image_get_width();
  height = imlib_image_get_height();
  */
  
  font = imlib_load_font(widget_item->font);
  if (!font) {
    printf("Unable to load font\n");
    exit(1);
  }
  imlib_context_set_font(font);
 
  imlib_get_text_size(widget_item->label, &item->width, &item->height);  

  if (widget_item->image) {
    imlib_context_set_image(widget_item->image);
    imlib_free_image();
  }
  
  widget_item->image = imlib_create_image(item->width, item->height);
  imlib_context_set_image(widget_item->image);
  imlib_image_set_has_alpha(1);
  memset(imlib_image_get_data(), 0, item->width * item->height * 4);
  
  imlib_context_set_color(255, 255, 255, 255);  
  imlib_text_draw(0, 0, "\xEF\x83\x88"); 
  imlib_context_set_color(255, 0, 0, 255);  
  imlib_text_draw(0, 0, widget_item->label); 

  texture_initialize(&widget_item->texture);
  if (!widget_item->texture.texture_id) {
    glGenTextures(1, &widget_item->texture.texture_id);
  }
  
  glBindTexture(GL_TEXTURE_2D, widget_item->texture.texture_id);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_BLUE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_GREEN);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_RED);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_ALPHA);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, item->width, item->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, imlib_image_get_data_for_reading_only());
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  gl_check_error("texture_from_pixmap4");
 
  item_type_base.update(item);
  imlib_free_font();
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

Item *item_get_widget(char *label, char *font) {
  WidgetItem *item;
  
  imlib_add_path_to_font_path("./fontawesome");

  item = (WidgetItem *) malloc(sizeof(WidgetItem));
  item->label = label;
  item->font = font;
  item->image = NULL;
  item->base.coords[0] = .25;
  item->base.coords[1] = .25;
  item->base.coords[2] = .25;
  item->base.coords[3] = .25;
  item->base.width = 1;
  item->base.height = 1;
  item->base.type = &item_type_widget;
  item_add((Item *) item);
  item->base.is_mapped = 1;

  item_type_widget.update((Item*) item);

  return item;
}
