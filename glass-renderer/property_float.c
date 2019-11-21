#include "property_float.h"
#include "shader.h"
#include "rendering.h"
#include "debug.h"

#define FL(value) *((float *) &value)
void property_float_init(PropertyTypeHandler *prop) { prop->type = XA_FLOAT; prop->name = AnyPropertyType; }
void property_float_load(Property *prop) {
  prop->data = realloc(prop->data, sizeof(float) * prop->nitems);
  for (int i = 0; i < prop->nitems; i++) {
    ((float *) prop->data)[i] = FL(prop->values.dwords[i]);
  }
}

void property_float_free(Property *prop) {
  if (prop->data) free(prop->data);
}

void property_float_to_gl(Property *prop, Rendering *rendering) {
  float *values = (float *) prop->data;
  PropertyProgramCache *prop_cache = &prop->programs[rendering->program_cache_idx];
  if (prop_cache->location == -1) return;
  switch (prop->nitems) {
    case 1: glUniform1f(prop_cache->location, values[0]); break;
    case 2: glUniform2f(prop_cache->location, values[0], values[1]); break;
    case 3: glUniform3f(prop_cache->location, values[0], values[1], values[2]); break;
    case 4:
      glUniform4f(prop_cache->location, values[0], values[1], values[2], values[3]);
      break;
  }
}
void property_float_print(Property *prop, FILE *fp) {
  float *values = (float *) prop->data;
  fprintf(fp, "%ld.%s=<int>", prop->window, prop->name_str);
  for (int i = 0; i <prop->nitems; i++) {
    if (i > 0) fprintf(fp, ",");
    fprintf(fp, "%f", values[i]);
  }
  fprintf(fp, "\n");
}
void property_float_load_program(Property *prop, Rendering *rendering) {
  PropertyProgramCache *prop_cache = &prop->programs[rendering->program_cache_idx];
  DEBUG("prop", "%ld[%ld].%s %s (float) [%d]\n",
        prop->window, prop_cache->program, prop_cache->name_str,
        (prop_cache->location != -1) ? "enabled" : "disabled", prop->nitems);
}
void property_float_free_program(Property *prop, size_t index) {
}
PropertyTypeHandler property_float = {&property_float_init, &property_float_load, &property_float_free, &property_float_to_gl, &property_float_print, &property_float_load_program, &property_float_free_program};
