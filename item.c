#include "glapi.h"
#include "xapi.h"
#include "item.h"
#include "wm.h"
#include <limits.h>

void item_type_base_destructor(Item *item) {}
void item_type_base_draw(Item *item) {
  if (item->is_mapped) {
    ItemShader *shader = (ItemShader *) item->type->get_shader(item);

    glUniform1i(shader->picking_mode_attr, 0);
    glUniform4fv(shader->screen_attr, 1, screen);
    
    glUniform1f(shader->window_id_attr, (float) item->id / (float) INT_MAX);
    
    glEnableVertexAttribArray(shader->coords_attr);
    glBindBuffer(GL_ARRAY_BUFFER, item->coords_vbo);
    glVertexAttribPointer(shader->coords_attr, 4, GL_FLOAT, GL_FALSE, 0, 0);

    glDrawArrays(GL_POINTS, 0, 1);
  }
}
void item_type_base_update(Item *item) {
  if (item->coords_vbo == -1) {
    glGenBuffers(1, &item->coords_vbo);
  }
  glBindBuffer(GL_ARRAY_BUFFER, item->coords_vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(item->coords), item->coords, GL_STATIC_DRAW);

  item->_width = item->width;
  item->_height = item->height;
  item->_coords[0] = item->coords[0];
  item->_coords[1] = item->coords[1];
  item->_coords[2] = item->coords[2];
  item->_coords[3] = item->coords[3];
  item->_is_mapped = item->is_mapped;
}

Shader *item_type_base_get_shader(Item *) {
  return NULL;
}

ItemType item_type_base = {
  &item_type_base_destructor,
  &item_type_base_draw,
  &item_type_base_update,
  &item_type_base_get_shader
};

Item **items_all = NULL;
size_t items_all_usage = 0;
size_t items_all_size = 0;
size_t items_all_id = 0;

Item *item_get(int id) {
  Item *item;
  size_t idx = 0;

  if (items_all) {
    for (; items_all[idx] && items_all[idx]->id != id; idx++);
    if (items_all[idx]) return items_all[idx];
  }

  return NULL;
}

void item_add(Item *item) {
  if (items_all_usage+1 > items_all_size) {
   if (!items_all_size) items_all_size = 8;
   items_all_size *=2;
   items_all = realloc(items_all, sizeof(Item *) * items_all_size);
  }

  item->id = ++items_all_id;
  item->coords_vbo = -1;
  item->is_mapped = False;
  items_all[items_all_usage] = item;
  items_all[items_all_usage+1] = NULL;

  items_all_usage++;
}

void item_remove(Item *item) {
  size_t idx;

  for (idx = 0; items_all[idx] && items_all[idx] != item; idx++);
  if (!items_all[idx]) return;
  memmove(items_all+idx, items_all+idx+1, sizeof(Item *) * (items_all_size-idx-1));
  items_all_usage--;
}
