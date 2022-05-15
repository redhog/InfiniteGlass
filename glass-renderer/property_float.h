#ifndef PROPERTY_FLOAT
#define PROPERTY_FLOAT

#include "property.h"

extern void property_float_init(PropertyTypeHandler *prop);
extern void property_float_load(Property *prop);
extern void property_float_free(Property *prop);
extern void property_float_to_gl(Property *prop, Rendering *rendering);
extern void property_float_print(Property *prop, int indent, FILE *fp);
extern void property_float_load_program(Property *prop, Rendering *rendering);
extern void property_float_free_program(Property *prop, size_t index);
extern PropertyTypeHandler property_float;


#endif
