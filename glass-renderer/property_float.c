#include "property_float.h"
#include "shader.h"
#include "rendering.h"
#include "debug.h"
#include <math.h>

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
  PropertyProgramCache *prop_cache = &prop->programs[rendering->program_cache_idx];
  if (prop_cache->location == -1) return;
  
  if (prop_cache->is_uniform) {
    float *data = (float *) prop->data;
    #define D(idx) ((idx < prop->nitems) ? data[idx] : nanf("initial"))
    switch (prop_cache->type) {
      case GL_FLOAT: glUniform1f(prop_cache->location, D(0)); break;
      case GL_FLOAT_VEC2: glUniform2f(prop_cache->location, D(0), D(1)); break;
      case GL_FLOAT_VEC3: glUniform3f(prop_cache->location, D(0), D(1), D(2)); break;
      case GL_FLOAT_VEC4: glUniform4f(prop_cache->location, D(0), D(1), D(2), D(3)); break;
    }
  } else {
    glBindBuffer(GL_ARRAY_BUFFER, prop_cache->buffer);
    glEnableVertexAttribArray(prop_cache->location);

    GLint size;
    switch (prop_cache->type) {
      case GL_FLOAT: size=1; break;
      case GL_FLOAT_VEC2: size=2; break;
      case GL_FLOAT_VEC3: size=3; break;
      case GL_FLOAT_VEC4: size=4; break;
    }
    glVertexAttribPointer(prop_cache->location, size, GL_FLOAT, False, 0, 0);

    glBindBuffer(GL_ARRAY_BUFFER, prop_cache->buffer);
    glBufferData(GL_ARRAY_BUFFER, prop->nitems * sizeof(float), prop->data, GL_STATIC_DRAW);

    if (rendering->array_length < prop->nitems / size) {
      rendering->array_length = prop->nitems / size;
    }
  }
}
void property_float_print(Property *prop, FILE *fp) {
  float *values = (float *) prop->data;
  fprintf(fp, "%ld.%s=<float>", prop->window, prop->name_str);
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
