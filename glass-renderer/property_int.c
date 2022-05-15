#include "property_int.h"
#include "shader.h"
#include "rendering.h"
#include "debug.h"
#include "glapi.h"

void property_int_init(PropertyTypeHandler *prop) { prop->type = XA_INTEGER; prop->name = AnyPropertyType; }
void property_int_load(Property *prop) {}
void property_int_free(Property *prop) {}
void property_int_to_gl(Property *prop, Rendering *rendering) {
  PropertyProgramCache *prop_cache = &prop->programs[rendering->program_cache_idx];
  if (prop_cache->location == -1) return;

  if (prop_cache->is_uniform) {
    uint32_t *data = prop->values.dwords;
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
void property_int_print(Property *prop, int indent, FILE *fp) {
  fprintf(fp, "%s%s: !int [", get_indent(indent), prop->name_str);
  for (int i = 0; i <prop->nitems; i++) {
    if (i > 0) fprintf(fp, ", ");
    fprintf(fp, "%i", prop->values.dwords[i]);
  }
  fprintf(fp, "]\n");
}
void property_int_load_program_none(Property *prop, Rendering *rendering) {}
void property_int_load_program_print(Property *prop, Rendering *rendering) {
  PropertyProgramCache *prop_cache = &prop->programs[rendering->program_cache_idx];
  DEBUG("prop", "%ld[%ld].%s %s (int) [%d]\n",
        prop->window, prop_cache->program, prop_cache->name_str,
        (prop_cache->location != -1) ? "enabled" : "disabled", prop->nitems);
}
void property_int_load_program(Property *prop, Rendering *rendering) {
  if (DEBUG_ENABLED("prop")) {
    property_int.load_program = &property_int_load_program_print;
    property_int.load_program(prop, rendering);
  } else {
    property_int.load_program = &property_int_load_program_none;
  }
}
void property_int_free_program(Property *prop, size_t index) {
}
PropertyTypeHandler property_int = {
  .init=&property_int_init,
  .load=&property_int_load,
  .free=&property_int_free,
  .to_gl=&property_int_to_gl,
  .print=&property_int_print,
  .load_program=&property_int_load_program,
  .free_program=&property_int_free_program
};
