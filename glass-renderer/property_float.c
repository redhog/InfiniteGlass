#include "property_float.h"
#include "shader.h"
#include "rendering.h"
#include "debug.h"
#include <math.h>

#define FL(value) *((float *) &value)
void property_float_init(PropertyTypeHandler *prop) { prop->type = XA_FLOAT; prop->name = AnyPropertyType; }

void property_float_to_gl(Property *prop, Rendering *rendering) {
  PropertyProgramCache *prop_cache = &prop->programs[rendering->program_cache_idx];
  if (prop_cache->location == -1) return;
  
  if (prop_cache->is_uniform) {
    float *data = (float *) prop->values.dwords;
    #define D(idx) ((idx < prop->nitems) ? data[idx] : nanf("initial"))
    
    if (rendering->print) {
      switch (prop_cache->type) {
        case GL_FLOAT: printf("%s%s: %f\n", get_indent(rendering->indent), prop->name_str, D(0)); break;
        case GL_FLOAT_VEC2: printf("%s%s: [%f, %f]\n", get_indent(rendering->indent), prop->name_str, D(0), D(1)); break;
        case GL_FLOAT_VEC3: printf("%s%s: [%f, %f, %f]\n", get_indent(rendering->indent), prop->name_str, D(0), D(1), D(2)); break;
        case GL_FLOAT_VEC4: printf("%s%s: [%f, %f, %f, %f]\n", get_indent(rendering->indent), prop->name_str, D(0), D(1), D(2), D(3)); break;
      }
    }
    
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

    if (rendering->print) {
      printf("%s%s: !floatarray {size: %d, length: %ld}\n", get_indent(rendering->indent), prop->name_str, size, prop->nitems);
    }
    
    glVertexAttribPointer(prop_cache->location, size, GL_FLOAT, False, 0, 0);

    glBindBuffer(GL_ARRAY_BUFFER, prop_cache->buffer);
    glBufferData(GL_ARRAY_BUFFER, prop->nitems * sizeof(float), prop->values.dwords, GL_STATIC_DRAW);

    if (rendering->array_length < prop->nitems / size) {
      rendering->array_length = prop->nitems / size;
    }
  }
}
void property_float_print(Property *prop, int indent, FILE *fp, int detail) {
  float *values = (float *) prop->values.dwords;
  int limit = (detail == 0 && prop->nitems > 10) ? 10 : prop->nitems;
  fprintf(fp, "%s%s: !FLOAT [", get_indent(indent), prop->name_str);
  for (int i = 0; i < limit; i++) {
    if (i > 0) fprintf(fp, ", ");
    fprintf(fp, "%f", values[i]);
  }
  if (limit < prop->nitems) {
    fprintf(fp, "] # Truncated\n");
  } else {
    fprintf(fp, "]\n");
  }
}
void property_float_load_program(Property *prop, Rendering *rendering) {
  PropertyProgramCache *prop_cache = &prop->programs[rendering->program_cache_idx];
  DEBUG("prop", "%ld[%ld].%s %s (float) [%d]\n",
        prop->window, prop_cache->program, prop_cache->name_str,
        (prop_cache->location != -1) ? "enabled" : "disabled", prop->nitems);
}
void property_float_free_program(Property *prop, size_t index) {
}
PropertyTypeHandler property_float = {
  .init=&property_float_init,
  .to_gl=&property_float_to_gl,
  .print=&property_float_print,
  .load_program=&property_float_load_program,
  .free_program=&property_float_free_program
};
