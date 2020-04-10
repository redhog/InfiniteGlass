#include "property_views.h"
#include "property_atom.h"
#include "view.h"
#include "wm.h"
#include "debug.h"

void property_views_init(PropertyTypeHandler *prop) { prop->type = XA_ATOM; prop->name = ATOM("IG_VIEWS"); }
void property_views_load(Property *prop) {
  if (prop->window != root) return;
  view_free_all(views);
  views = view_load_all();
}

PropertyTypeHandler property_views = {
  .init=&property_views_init,
  .load=&property_views_load,
  .free=&property_atom_free,
  .to_gl=&property_atom_to_gl,
  .print=&property_atom_print,
  .load_program=&property_atom_load_program,
  .free_program=&property_atom_free_program
};
