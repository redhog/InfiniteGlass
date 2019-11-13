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
  if (prop->program != rendering->shader->program) {
    prop->program = rendering->shader->program;
    prop->location = glGetUniformLocation(prop->program, prop->name_str);
    DEBUG("prop", "%ld.%s %s (float) [%d]\n", rendering->shader->program, prop->name_str, (prop->location != -1) ? "enabled" : "disabled", prop->nitems);
  }
  if (prop->location == -1) return;
  switch (prop->nitems) {
    case 1: glUniform1f(prop->location, values[0]); break;
    case 2: glUniform2f(prop->location, values[0], values[1]); break;
    case 3: glUniform3f(prop->location, values[0], values[1], values[2]); break;
    case 4:
      glUniform4f(prop->location, values[0], values[1], values[2], values[3]);
      break;
  }
}
void property_float_print(Property *prop, FILE *fp) {
  float *values = (float *) prop->data;
  fprintf(fp, "%s=<int>", prop->name_str);
  for (int i = 0; i <prop->nitems; i++) {
    if (i > 0) fprintf(fp, ",");
    fprintf(fp, "%f", values[i]);
  }
  fprintf(fp, "\n");
}
PropertyTypeHandler property_float = {&property_float_init, &property_float_load, &property_float_free, &property_float_to_gl, &property_float_print};
