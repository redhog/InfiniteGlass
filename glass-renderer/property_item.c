#include "property_item.h"
#include "shader.h"
#include "rendering.h"
#include "item.h"
#include "debug.h"
#include "xapi.h"

void property_item_init(PropertyTypeHandler *prop) { prop->type = XInternAtom(display, "IG_ITEM", False); prop->name = AnyPropertyType; }
void property_item_load(Property *prop) {}
void property_item_free(Property *prop) {}
void property_item_to_gl(Property *prop, Rendering *rendering) {}
void property_item_print(Property *prop, FILE *fp) {
  fprintf(fp, "%ld.%s=<item>", prop->window, prop->name_str);
  for (int i = 0; i <prop->nitems; i++) {
    if (i > 0) fprintf(fp, ",");
    fprintf(fp, "%d", prop->values.dwords[i]);
  }
  fprintf(fp, "\n");
}
void property_item_load_program(Property *prop, Rendering *rendering) {
  DEBUG("prop", "%ld[%ld].%s (item) [enabled]\n",
        prop->window, rendering->shader->program, prop->name_str);
}
void property_item_free_program(Property *prop, size_t index) {
}
void property_item_draw(Property *prop, Rendering *rendering) {
  rendering->item = item_get_from_window((Window) prop->values.dwords[0], True);
  if (rendering->item == rendering->parent_item) return;
  if (!rendering->item->prop_item_layer) return;
  if (!rendering->item->prop_item_layer->values.dwords) return;
  if (!rendering->parent_item->prop_layer) return;
  if (!rendering->parent_item->prop_layer->values.dwords) return;
  if (rendering->item->prop_item_layer->values.dwords[0] != rendering->parent_item->prop_layer->values.dwords[0]) return;
  
  item_draw(rendering);
}

PropertyTypeHandler property_item = {&property_item_init, &property_item_load, &property_item_free, &property_item_to_gl, &property_item_print, &property_item_load_program, &property_item_free_program, &property_item_draw};
