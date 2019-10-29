#include "property.h"

Property *property_allocate(Atom name) {
  Property *prop = malloc(sizeof(Property));
  prop->name = name;
  prop->program = -1;
  prop->name_str = NULL;
  prop->values.bytes = NULL;
  return prop;
}

void property_load(Property *prop, Window window) {
  unsigned long bytes_after_return;
  unsigned char *prop_return;

  if (prop->values.bytes) {
    XFree(prop->values.bytes);
  }
  if (prop->name_str) {
    XFree(prop->values.bytes);
  }
  XGetWindowProperty(display, window, prop->name, 0, 0, 0, AnyPropertyType, &prop->type, &prop->format, &prop->nitems, &bytes_after_return, &prop_return);
  XFree(prop_return);
  if (prop->type == None) return;
  XGetWindowProperty(display, window, prop->name, 0, bytes_after_return, 0, prop->type,
                     &prop->type, &prop->format, &prop->nitems, &bytes_after_return, &prop->values.bytes);
  property_type_get(prop->type)->load(prop);
  prop->name_str = XGetAtomName(display, prop->name);
}

void property_free(Property *prop) {
  property_type_get(prop->type)->free(prop);
  if (prop->name_str) XFree(prop->name_str);
  if (prop->values.bytes) XFree(prop->values.bytes);
  free(prop);
}


List *properties_load(Window window) {
  List *prop_list = list_create();
  int nr_props;
  Atom *prop_names = XListProperties(display, window, &nr_props);
  for (int i = 0; i < nr_props; i++) {
    Property *prop = property_allocate(prop_names[i]);
    property_load(prop, window);
    list_append(prop_list, (void *) prop);
  }
  return prop_list;
}

void properties_update(List *properties, Window window, Atom name) {
  for (size_t i = 0; i < properties->count; i++) {
    Property *prop = (Property *) properties->entries[i];
    if (prop->name == name) {
      property_load(prop, window);
      return;
    }   
  }
  Property *prop = property_allocate(name);
  property_load(prop, window);
  list_append(properties, (void *) prop);
}

void properties_free(List *properties) {
  for (size_t i = 0; i < properties->count; i++) {
    Property *prop = (Property *) properties->entries[i];
    property_free(prop);
  }
  list_destroy(properties);
}


List *property_types = NULL;

void property_type_register(PropertyTypeHandler *handler) {
  if (!property_types) {
    property_types = list_create();
  }
  handler->init(handler);
  list_append(property_types, (void *) handler);
}


PropertyTypeHandler *property_type_get(Atom type) {
  if (!property_types) return NULL;
  for (size_t i = 0; i < property_types->count; i++) {
    PropertyTypeHandler *property_type = property_types->entries[i];
    if (property_type->type == type) {
      return property_type;
    }
  }
  return NULL;
}


void property_int_init(PropertyTypeHandler *prop) { prop->type = XA_INTEGER; }
void property_int_load(Property *prop) {}
void property_int_free(Property *prop) {}
void property_int_to_gl(Property *prop, Shader *shader) {
  if (prop->program != shader->program) {
    prop->program = shader->program;
    prop->location = glGetUniformLocation(prop->program, prop->name_str);
  }
  if (prop->location == -1) return;
  switch (prop->nitems) {
    case 1: glUniform1i(prop->location, prop->values.dwords[0]); break;
    case 2: glUniform2i(prop->location, prop->values.dwords[0], prop->values.dwords[1]); break;
    case 3: glUniform3i(prop->location, prop->values.dwords[0], prop->values.dwords[1], prop->values.dwords[2]); break;
    case 4: glUniform4i(prop->location, prop->values.dwords[0], prop->values.dwords[1], prop->values.dwords[2], prop->values.dwords[3]); break;
  }
}
PropertyTypeHandler property_int = {&property_int_init, &property_int_load, &property_int_free, &property_int_to_gl};

#define FL(value) *((float *) &value)
void property_float_init(PropertyTypeHandler *prop) { prop->type = XA_FLOAT; }
void property_float_load(Property *prop) {}
void property_float_free(Property *prop) {}
void property_float_to_gl(Property *prop, Shader *shader) {
  if (prop->program != shader->program) {
    prop->program = shader->program;
    prop->location = glGetUniformLocation(prop->program, prop->name_str);
  }
  if (prop->location == -1) return;
  switch (prop->nitems) {
    case 1: glUniform1f(prop->location, FL(prop->values.dwords[0])); break;
    case 2: glUniform2f(prop->location, FL(prop->values.dwords[0]), FL(prop->values.dwords[1])); break;
    case 3: glUniform3f(prop->location, FL(prop->values.dwords[0]), FL(prop->values.dwords[1]), FL(prop->values.dwords[2])); break;
    case 4:
      glUniform4f(prop->location, FL(prop->values.dwords[0]), FL(prop->values.dwords[1]), FL(prop->values.dwords[2]), FL(prop->values.dwords[3]));
      break;
  }
}
PropertyTypeHandler property_float = {&property_float_init, &property_float_load, &property_float_free, &property_float_to_gl};
