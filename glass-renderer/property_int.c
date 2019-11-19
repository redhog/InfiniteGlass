#include "property_int.h"
#include "shader.h"
#include "rendering.h"
#include "debug.h"

void property_int_init(PropertyTypeHandler *prop) { prop->type = XA_INTEGER; prop->name = AnyPropertyType; }
void property_int_load(Property *prop) {}
void property_int_free(Property *prop) {}
void property_int_to_gl(Property *prop, Rendering *rendering) {
  PropertyProgramCache *prop_cache = &prop->programs[rendering->program_cache_idx];
  ProgramCache *cache = &rendering->properties->programs[rendering->program_cache_idx];
   
  if (prop_cache->program != cache->program) {
    prop_cache->program = cache->program;
    prop_cache->name_str = realloc(prop_cache->name_str, strlen(cache->prefix) + strlen(prop->name_str) + 1);
    strcpy(prop_cache->name_str, cache->prefix);
    strcpy(prop_cache->name_str + strlen(cache->prefix), prop->name_str);
    prop_cache->location = glGetUniformLocation(cache->program, prop_cache->name_str);
    DEBUG("prop", "%ld.%s %s (int) [%d]\n", prop_cache->program, prop_cache->name_str, (prop_cache->location != -1) ? "enabled" : "disabled", prop->nitems);
  }
  if (prop_cache->location == -1) return;
  switch (prop->nitems) {
    case 1: glUniform1i(prop_cache->location, prop->values.dwords[0]); break;
    case 2: glUniform2i(prop_cache->location, prop->values.dwords[0], prop->values.dwords[1]); break;
    case 3: glUniform3i(prop_cache->location, prop->values.dwords[0], prop->values.dwords[1], prop->values.dwords[2]); break;
    case 4: glUniform4i(prop_cache->location, prop->values.dwords[0], prop->values.dwords[1], prop->values.dwords[2], prop->values.dwords[3]); break;
  }
}
void property_int_print(Property *prop, FILE *fp) {
  fprintf(fp, "%s=<int>", prop->name_str);
  for (int i = 0; i <prop->nitems; i++) {
    if (i > 0) fprintf(fp, ",");
    fprintf(fp, "%li", prop->values.dwords[i]);
  }
  fprintf(fp, "\n");
}
PropertyTypeHandler property_int = {&property_int_init, &property_int_load, &property_int_free, &property_int_to_gl, &property_int_print};
