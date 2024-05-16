#ifndef PROPERTY_INT
#define PROPERTY_INT

#include "property.h"

void property_int_init(PropertyTypeHandler *prop);
void property_int_load(Property *prop);
void property_int_free(Property *prop);
void property_int_to_gl(Property *prop, Rendering *rendering);
void property_int_print(Property *prop, int indent, FILE *fp, int detail);
void property_int_load_program_none(Property *prop, Rendering *rendering);
void property_int_load_program_print(Property *prop, Rendering *rendering);
void property_int_load_program(Property *prop, Rendering *rendering);
void property_int_free_program(Property *prop, size_t index);

extern PropertyTypeHandler property_int;

#endif
