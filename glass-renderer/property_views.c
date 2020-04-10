#include "property_views.h"
#include "property_atom.h"
#include "view.h"
#include "wm.h"
#include "debug.h"

int property_views_match(Property *prop) {
  if (prop->window != root) return -1;
  if (prop->type == XA_ATOM && prop->name == ATOM("IG_VIEWS")) return 2;
  if (strncmp("IG_VIEW_", prop->name_str, strlen("IG_VIEW_")) == 0) return 2;
  return -1;
}
 
void property_views_load(Property *prop) {
  if (prop->window != root) return;
  if (prop->name == ATOM("IG_VIEWS")) {
    view_free_all(views);
    views = view_load_all();
  } else {
    if (views) {
      for (size_t idx = 0; idx < views->count; idx++) {
        View *v = (View *) views->entries[idx];
        if (prop->name == v->attr_layer) {
          view_load_layer(v);
        } else if (prop->name == v->attr_view) {
          view_load_screen(v);
        }
      }
    }
  }
}

PropertyTypeHandler property_views = {
  .match=&property_views_match,
  .load=&property_views_load,
  .free=&property_atom_free,
  .to_gl=&property_atom_to_gl,
  .print=&property_atom_print,
  .load_program=&property_atom_load_program,
  .free_program=&property_atom_free_program
};
