#include "property_window.h"
#include "shader.h"
#include "rendering.h"
#include "debug.h"
#include "xapi.h"

void property_window_init(PropertyTypeHandler *prop) { prop->type = XA_WINDOW; prop->name = AnyPropertyType; }
void property_window_load(Property *prop) {}
void property_window_free(Property *prop) {}
void property_window_to_gl(Property *prop, Rendering *rendering) {
  PropertyProgramCache *prop_cache = &prop->programs[rendering->program_cache_idx];
  if (prop_cache->location == -1) return;
  if (prop_cache->is_uniform) {
    #define D(idx) prop->values.dwords[idx]
   
    if (rendering->print) {
      switch (prop->nitems) {
        case 1: printf("%s%s: %d\n", get_indent(rendering->indent), prop->name_str, D(0)); break;
        case 2: printf("%s%s: [%d, %d]\n", get_indent(rendering->indent), prop->name_str, D(0), D(1)); break;
        case 3: printf("%s%s: [%d, %d, %d]\n", get_indent(rendering->indent), prop->name_str, D(0), D(1), D(2)); break;
        case 4: printf("%s%s: [%d, %d, %d, %d]\n", get_indent(rendering->indent), prop->name_str, D(0), D(1), D(2), D(3)); break;
      }
    }
   
    switch (prop->nitems) {
      case 1: glUniform1i(prop_cache->location, D(0)); break;
      case 2: glUniform2i(prop_cache->location, D(0), D(1)); break;
      case 3: glUniform3i(prop_cache->location, D(0), D(1), D(2)); break;
      case 4: glUniform4i(prop_cache->location, D(0), D(1), D(2), D(3)); break;
    }
  } else {
  }
}
void property_window_print(Property *prop, int indent, FILE *fp, int detail) {
  fprintf(fp, "%s%s: !WINDOW [", get_indent(indent), prop->name_str);
  for (int i = 0; i <prop->nitems; i++) {
    if (i > 0) fprintf(fp, ", ");
    fprintf(fp, "%d", prop->values.dwords[i]);
  }
  fprintf(fp, "]\n");
}
void property_window_load_program(Property *prop, Rendering *rendering) {
  PropertyProgramCache *prop_cache = &prop->programs[rendering->program_cache_idx];
  DEBUG("prop", "%ld[%ld].%s %s (window) [%d]\n",
        prop->window, rendering->shader->program, prop->name_str,
        (prop_cache->location != -1) ? "enabled" : "disabled", prop->nitems);
}
void property_window_free_program(Property *prop, size_t index) {
}
PropertyTypeHandler property_window = {
  .init=&property_window_init,
  .load=&property_window_load,
  .free=&property_window_free,
  .to_gl=&property_window_to_gl,
  .print=&property_window_print,
  .load_program=&property_window_load_program,
  .free_program=&property_window_free_program
};
