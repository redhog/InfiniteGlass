#include "property_atom.h"
#include "shader.h"
#include "rendering.h"
#include "debug.h"
#include "xapi.h"

void property_atom_init(PropertyTypeHandler *prop) { prop->type = XA_ATOM; prop->name = AnyPropertyType; }
void property_atom_load(Property *prop) {}
void property_atom_free(Property *prop) {}
void property_atom_to_gl(Property *prop, Rendering *rendering) {
  PropertyProgramCache *prop_cache = &prop->programs[rendering->program_cache_idx];
  if (prop_cache->location == -1) return;
  switch (prop->nitems) {
    case 1: glUniform1i(prop_cache->location, prop->values.dwords[0]); break;
    case 2: glUniform2i(prop_cache->location, prop->values.dwords[0], prop->values.dwords[1]); break;
    case 3: glUniform3i(prop_cache->location, prop->values.dwords[0], prop->values.dwords[1], prop->values.dwords[2]); break;
    case 4: glUniform4i(prop_cache->location, prop->values.dwords[0], prop->values.dwords[1], prop->values.dwords[2], prop->values.dwords[3]); break;
  }
}
void property_atom_print(Property *prop, FILE *fp) {
  fprintf(fp, "%ld.%s=<atom>", prop->window, prop->name_str);
  for (int i = 0; i <prop->nitems; i++) {
    if (i > 0) fprintf(fp, ",");
    char *name = XGetAtomName(display, (Atom) prop->values.dwords[i]);
    fprintf(fp, "%s", name);
    XFree(name);
  }
  fprintf(fp, "\n");
}
void property_atom_load_program(Property *prop, Rendering *rendering) {
  PropertyProgramCache *prop_cache = &prop->programs[rendering->program_cache_idx];
  DEBUG("prop", "%ld[%ld].%s %s (atom) [%d]\n",
        prop->window, rendering->shader->program, prop->name_str,
        (prop_cache->location != -1) ? "enabled" : "disabled", prop->nitems);
}
void property_atom_free_program(Property *prop, size_t index) {
}
PropertyTypeHandler property_atom = {&property_atom_init, &property_atom_load, &property_atom_free, &property_atom_to_gl, &property_atom_print, &property_atom_load_program, &property_atom_free_program};