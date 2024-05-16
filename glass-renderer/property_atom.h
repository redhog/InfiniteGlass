#ifndef PROPERTY_ATOM_H
#define PROPERTY_ATOM_H

#include "property.h"

extern void property_atom_init(PropertyTypeHandler *prop);
extern void property_atom_load(Property *prop);
extern void property_atom_free(Property *prop);
extern void property_atom_to_gl(Property *prop, Rendering *rendering);
extern void property_atom_print(Property *prop, int indent, FILE *fp, int detail);
extern void property_atom_load_program(Property *prop, Rendering *rendering);
extern void property_atom_free_program(Property *prop, size_t index);
extern PropertyTypeHandler property_atom;

#endif
