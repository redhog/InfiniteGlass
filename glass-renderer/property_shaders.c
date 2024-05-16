#include "property_shaders.h"
#include "property_atom.h"
#include "view.h"
#include "wm.h"
#include "debug.h"

int property_shaders_match(Property *prop) {
  if (prop->window != root) return -1;
  if (prop->type == XA_ATOM && prop->name == ATOM("IG_SHADERS")) return 2;
  if (strncmp("IG_SHADER_", prop->name_str, strlen("IG_SHADER_")) == 0) return 2;
  return -1;
}
 
void property_shaders_load(Property *prop) {
  if (prop->window != root) return; // This shouldn't happen, right?

  // This could potentially be optimized to only load changed shaders...
  // if (prop->name == ATOM("IG_SHADERS")) {

  shader_free_all(shaders);
  shaders = shader_load_all();  
}

void property_shaders_free(Property *prop) {
}

void property_shaders_print(Property *prop, int indent, FILE *fp, int detail) {
  if (prop->type == XA_ATOM) {
   property_atom_print(prop, indent, fp, detail);
  } else if (prop->type == XA_STRING) {
    fprintf(fp, "%s%s: !SHADER\n", get_indent(indent), prop->name_str);
  }
}

void property_shaders_to_gl(Property *prop, Rendering *rendering) {
  if (prop->type == XA_ATOM) {
    property_atom_to_gl(prop, rendering);
  }
}
void property_shaders_load_program(Property *prop, Rendering *rendering) {
  if (prop->type == XA_ATOM) {
    property_atom_load_program(prop, rendering);
  }
}
void property_shaders_free_program(Property *prop, size_t index) {
  if (prop->type == XA_ATOM) {
    property_atom_free_program(prop, index);
  }
}

PropertyTypeHandler property_shaders = {
  .match=&property_shaders_match,
  .load=&property_shaders_load,
  .free=&property_shaders_free,
  .print=&property_shaders_print,
  .to_gl=&property_shaders_to_gl,
  .load_program=&property_shaders_load_program,
  .free_program=&property_shaders_free_program
};
