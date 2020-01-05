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

  unsigned long *data = prop->values.dwords;
  #define D(idx) ((idx < prop->nitems) ? data[idx] : -1)
  switch (prop_cache->type) {
    case GL_INT: glUniform1i(prop_cache->location, D(0)); break;
    case GL_INT_VEC2: glUniform2i(prop_cache->location, D(0), D(1)); break;
    case GL_INT_VEC3: glUniform3i(prop_cache->location, D(0), D(1), D(2)); break;
    case GL_INT_VEC4: glUniform4i(prop_cache->location, D(0), D(1), D(2), D(3)); break;
  }
}
void property_int_print(Property *prop, FILE *fp) {
  fprintf(fp, "%ld.%s=<int>", prop->window, prop->name_str);
  for (int i = 0; i <prop->nitems; i++) {
    if (i > 0) fprintf(fp, ",");
    fprintf(fp, "%li", prop->values.dwords[i]);
  }
  fprintf(fp, "\n");
}
void property_int_load_program(Property *prop, Rendering *rendering) {
  PropertyProgramCache *prop_cache = &prop->programs[rendering->program_cache_idx];
  DEBUG("prop", "%ld[%ld].%s %s (int) [%d]\n",
        prop->window, prop_cache->program, prop_cache->name_str,
        (prop_cache->location != -1) ? "enabled" : "disabled", prop->nitems);
}
void property_int_free_program(Property *prop, size_t index) {
}
PropertyTypeHandler property_int = {&property_int_init, &property_int_load, &property_int_free, &property_int_to_gl, &property_int_print, &property_int_load_program, &property_int_free_program};
