#include "item_test.h"
#include "item_window_shader.h"
#include "texture.h"
#include "glapi.h"
#include "xapi.h"
#include "wm.h"
#include <Imlib2.h>

void item_type_test_destructor(Item *item) {}

void item_type_test_draw(View *view, Item *item) {
  gl_check_error("item_type_test_draw1");

  ItemWindowShader *shader = (ItemWindowShader *) item->type->get_shader(item);

  imlib_context_set_display(display);
  imlib_context_set_visual(DefaultVisual(display, DefaultScreen(display)));
  imlib_context_set_colormap(DefaultColormap(display, DefaultScreen(display)));

  Imlib_Image image = imlib_load_image("So_happy_smiling_cat.jpg");
  imlib_context_set_image(image);
  Pixmap pixmap = XCreatePixmap(display, root, imlib_image_get_width(), imlib_image_get_height(), DefaultDepth(display, DefaultScreen(display)));
  imlib_context_set_drawable(pixmap);
  imlib_render_image_on_drawable(0, 0);
  imlib_free_image();

  Texture texture;
  texture_initialize(&texture);
  texture_from_pixmap(&texture, pixmap);

  glUniform1i(shader->window_sampler_attr, 0);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texture.texture_id);
  glBindSampler(1, 0);
 
  gl_check_error("item_type_test_draw2");

  item_type_base.draw(view, item);

  gl_check_error("item_type_test_draw3");

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
