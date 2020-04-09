#include "property_shaders.h"
#include "property_atom.h"
#include "view.h"
#include "wm.h"
#include "debug.h"

void property_shaders_init(PropertyTypeHandler *prop) { prop->type = XA_ATOM; prop->name = ATOM("IG_SHADERS"); }
void property_shaders_load(Property *prop) {
  if (prop->window != root) return;
  shader_free_all(shaders);
  shaders = shader_load_all();       
}

PropertyTypeHandler property_shaders = {
  &property_shaders_init, &property_shaders_load,
  &property_atom_free, &property_atom_to_gl, &property_atom_print, &property_atom_load_program, &property_atom_free_program};
