#include "item_test.h"
#include "item_window_shader.h"
#include "texture.h"
#include "glapi.h"
#include "xapi.h"
#include "wm.h"
#include <Imlib2.h>

void item_type_test_destructor(Item *item) {}

void item_type_test_draw(View *view, Item *item) {
  int width, height;
 
  gl_check_error("item_type_test_draw1");

  ItemWindowShader *shader = (ItemWindowShader *) item->type->get_shader(item);

  imlib_context_set_display(display);
  imlib_context_set_visual(DefaultVisual(display, DefaultScreen(display)));
  imlib_context_set_colormap(DefaultColormap(display, DefaultScreen(display)));

  Imlib_Font font;
  imlib_add_path_to_font_path("/usr/share/fonts-font-awesome/fonts");

  /*
  Imlib_Image image = imlib_load_image("So_happy_smiling_cat.jpg");
  imlib_context_set_image(image);
  width = imlib_image_get_width();
  height = imlib_image_get_height();
  */
  
  font = imlib_load_font("fontawesome-webfont.ttf/128");
  if (!font) {
    printf("Unable to find font awesome\n");
    exit(1);
  }
  imlib_context_set_font(font);
 
  imlib_get_text_size("\xEF\x80\x8E", &width, &height);

  Pixmap pixmap = XCreatePixmap(display, root, width, height, DefaultDepth(display, DefaultScreen(display)));
  imlib_context_set_drawable(pixmap);

  Imlib_Image image = imlib_create_image(width, height);
  imlib_context_set_image(image);
  imlib_image_set_has_alpha(1);
  memset(imlib_image_get_data(), 0, width * height * 4);

  //imlib_context_set_color(0, 0, 0, 255);
  //imlib_image_fill_rectangle(0, 0, width, height);
  imlib_context_set_color(255, 0, 0, 255);
  
  imlib_text_draw(0, 0, "\xEF\x80\x8E"); 

  imlib_render_image_on_drawable(0, 0);

  
  Texture texture;
  texture_initialize(&texture);
  glGenTextures(1, &texture.texture_id);
  glBindTexture(GL_TEXTURE_2D, texture.texture_id);
  //glActiveTexture(GL_TEXTURE0);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_BLUE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_GREEN);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_RED);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_ALPHA);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, imlib_image_get_data_for_reading_only());
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  gl_check_error("texture_from_pixmap4");
 
 //texture_from_pixmap(&texture, pixmap);

  glUniform1i(shader->window_sampler_attr, 0);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texture.texture_id);
  glBindSampler(1, 0);
 
  gl_check_error("item_type_test_draw2");

  item_type_base.draw(view, item);

  gl_check_error("item_type_test_draw3");

  imlib_free_font();
  imlib_free_image();
  texture_destroy(&texture);
  XFreePixmap(display, pixmap);
}

void item_type_test_update(Item *item) {
  item_type_base.update(item);
}

Shader *item_type_test_get_shader(Item *item) {
  return (Shader *) item_window_shader_get();
}

ItemType item_type_test = {
  &item_type_test_destructor,
  &item_type_test_draw,
  &item_type_test_update,
  &item_type_test_get_shader
};

Item *item_get_test() {
  Item *item;
  
  item = (Item *) malloc(sizeof(Item));
  item->coords[0] = .5;
  item->coords[1] = .25;
  item->coords[2] = .25;
  item->coords[3] = .25;
  item->width = 100;
  item->height = 100;
  item->type = &item_type_test;
  item_add(item);
  item->is_mapped = 1;

  item_type_test.update((Item*) item);

  return item;
}
