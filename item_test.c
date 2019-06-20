#include "item_test.h"
#include "texture.h"
#include "glapi.h"
#include "xapi.h"

void item_type_test_destructor(Item *item) {}

void item_type_test_draw(Item *item) {
 /*
 texture_from_pixmap(&window_item->window_texture, window_item->window_pixmap);

 glUniform1i(window_sampler_attr, 0);
 glActiveTexture(GL_TEXTURE0);
 glBindTexture(GL_TEXTURE_2D, window_item->window_texture.texture_id);
 glBindSampler(1, 0);
 */
 
 item_type_base.draw(item);
}

void item_type_test_update(Item *item) {
  item_type_base.update(item);
}

ItemType item_type_test = {
  &item_type_test_destructor,
  &item_type_test_draw,
  &item_type_test_update
};

Item *item_get_test() {
  Item *item;
  
  item = (Item *) malloc(sizeof(Item));
  item->coords[0] = 0;
  item->coords[1] = 0;
  item->coords[2] = 1;
  item->coords[3] = 1;
  item->width = 100;
  item->height = 100;
  item->type = &item_type_test;
  item_add(item);

  return item;
}
