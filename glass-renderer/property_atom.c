#include "property_atom.h"
#include "shader.h"
#include "rendering.h"
#include "debug.h"
#include "xapi.h"

void property_atom_init(XConnection *conn, PropertyTypeHandler *prop) { prop->type = XA_ATOM; prop->name = AnyPropertyType; }
void property_atom_load(XConnection *conn, Property *prop) {}
void property_atom_free(XConnection *conn, Property *prop) {}
void property_atom_to_gl(Property *prop, Rendering *rendering) {
  PropertyProgramCache *prop_cache = &prop->programs[rendering->program_cache_idx];
  if (prop_cache->location == -1) return;
  if (prop_cache->is_uniform) {
    unsigned long *data = prop->values.dwords;
    #define D(idx) ((idx < prop->nitems) ? data[idx] : -1)
    switch (prop_cache->type) {
      case GL_INT: glUniform1i(prop_cache->location, D(0)); break;
      case GL_INT_VEC2: glUniform2i(prop_cache->location, D(0), D(1)); break;
      case GL_INT_VEC3: glUniform3i(prop_cache->location, D(0), D(1), D(2)); break;
      case GL_INT_VEC4: glUniform4i(prop_cache->location, D(0), D(1), D(2), D(3)); break;
    }
  } else {
  }
}
void property_atom_print(XConnection *conn, Property *prop, FILE *fp) {
  fprintf(fp, "%ld.%s=<atom>", prop->window, prop->name_str);
  for (int i = 0; i <prop->nitems; i++) {
    if (i > 0) fprintf(fp, ",");
    char *name = XGetAtomName(conn->display, (Atom) prop->values.dwords[i]);
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
