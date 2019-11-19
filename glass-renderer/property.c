#include "property.h"
#include "shader.h"
#include "rendering.h"
#include "debug.h"

Property *property_allocate(Properties *properties, Atom name) {
  Property *prop = malloc(sizeof(Property));
  prop->window = properties->window;
  prop->name = name;
  prop->name_str = XGetAtomName(display, prop->name);
  prop->values.bytes = NULL;
  for (size_t i = 0; i < PROGRAM_CACHE_SIZE; i++) {
    prop->programs[i].program = -1;
    prop->programs[i].data = NULL;
    prop->programs[i].name_str = NULL;
    prop->programs[i].location = -1;
  }
  prop->data = NULL;
  return prop;
}

Bool property_load(Property *prop) {
  unsigned long bytes_after_return;
  unsigned char *prop_return;

  unsigned char *old = prop->values.bytes;
  int old_nitems = prop->nitems;
  int old_format = prop->format;
  prop->values.bytes = NULL;

  XGetWindowProperty(display, prop->window, prop->name, 0, 0, 0, AnyPropertyType,
                     &prop->type, &prop->format, &prop->nitems, &bytes_after_return, &prop_return);
  XFree(prop_return);
  if (prop->type == None) {
    if (old) { XFree(old); return True; }
    return False;
  }
  XGetWindowProperty(display, prop->window, prop->name, 0, bytes_after_return, 0, prop->type,
                     &prop->type, &prop->format, &prop->nitems, &bytes_after_return, &prop->values.bytes);
  Bool changed = !old || old_nitems != prop->nitems || old_format != prop->format || memcmp(old, prop->values.bytes, prop->nitems * prop->format) != 0;

  if (old) XFree(old);
  if (!changed) return False;
  
  PropertyTypeHandler *type = property_type_get(prop->type, prop->name);
  if (type) type->load(prop);
  if (DEBUG_ENABLED("prop.changed")) {
    DEBUG("prop.changed", "");
    property_print(prop, stderr);
  }
  
  return True;
}

void property_free(Property *prop) {
  PropertyTypeHandler *type = property_type_get(prop->type, prop->name);
  if (type) type->free(prop);
  for (size_t i = 0; i < PROGRAM_CACHE_SIZE; i++) {
    if (type) type->free_program(prop, i);
    if (prop->programs[i].name_str) free(prop->programs[i].name_str);
  }
  if (prop->name_str) XFree(prop->name_str);
  if (prop->values.bytes) XFree(prop->values.bytes);
  free(prop);
}

void property_to_gl(Property *prop, Rendering *rendering) {
  PropertyProgramCache *prop_cache = &prop->programs[rendering->program_cache_idx];
  ProgramCache *cache = &rendering->properties->programs[rendering->program_cache_idx];
  PropertyTypeHandler *type = property_type_get(prop->type, prop->name);
  if (!type) return;
  
  if (prop_cache->program != cache->program) {
    if (prop_cache->program != -1) type->free_program(prop, rendering->program_cache_idx);
    prop_cache->program = cache->program;
    prop_cache->name_str = realloc(prop_cache->name_str, strlen(cache->prefix) + strlen(prop->name_str) + 1);
    strcpy(prop_cache->name_str, cache->prefix);
    strcpy(prop_cache->name_str + strlen(cache->prefix), prop->name_str);
    prop_cache->location = glGetUniformLocation(cache->program, prop_cache->name_str);
    type->load_program(prop, rendering);
  }
  type->to_gl(prop, rendering);
}

void property_print(Property *prop, FILE *fp) {
  PropertyTypeHandler *type = property_type_get(prop->type, prop->name);
  if (type) {
    type->print(prop, fp);
  } else {
    char *type_name = XGetAtomName(display, prop->type);
    fprintf(fp, "%s=<%s>\n", prop->name_str, type_name);
    XFree(type_name);
  }
}

Properties *properties_load(Window window) {
  Properties *properties = malloc(sizeof(Properties));
  properties->window = window;
  properties->programs_pos = 0; 
  properties->properties = list_create();
  int nr_props;
  Atom *prop_names = XListProperties(display, window, &nr_props);
  for (int i = 0; i < nr_props; i++) {
    Property *prop = property_allocate(properties, prop_names[i]);
    property_load(prop);
    list_append(properties->properties, (void *) prop);
  }
  XFree(prop_names);
  return properties;
}

Bool properties_update(Properties *properties, Atom name) {
  for (size_t i = 0; i < properties->properties->count; i++) {
    Property *prop = (Property *) properties->properties->entries[i];
    if (prop->name == name) {
      return property_load(prop);
    }   
  }
  Property *prop = property_allocate(properties, name);
  property_load(prop);
  list_append(properties->properties, (void *) prop);
  return True;
}

void properties_free(Properties *properties) {
  for (size_t i = 0; i < properties->properties->count; i++) {
    Property *prop = (Property *) properties->properties->entries[i];
    property_free(prop);
  }
  list_destroy(properties->properties);
  free(properties);
}

int properties_get_program_cache_idx(Rendering *rendering) {
  for (size_t i = 0; i < PROGRAM_CACHE_SIZE; i++) {
    if (rendering->properties->programs[i].program == rendering->shader->program) {
      return i;
    }
  }
  rendering->properties->programs_pos = (rendering->properties->programs_pos + 1) % PROGRAM_CACHE_SIZE;
  return rendering->properties->programs_pos;
}

void properties_to_gl(Properties *properties, char *prefix, Rendering *rendering) {
  rendering->properties = properties;
  rendering->properties_prefix = prefix;
  rendering->program_cache_idx = properties_get_program_cache_idx(rendering);
  properties->programs[rendering->program_cache_idx].program = rendering->shader->program;
  properties->programs[rendering->program_cache_idx].prefix = prefix;
  
  gl_check_error("properties_to_gl");
  for (size_t i = 0; i < properties->properties->count; i++) {
    Property *prop = (Property *) properties->properties->entries[i];
    property_to_gl(prop, rendering);
    gl_check_error(prop->name_str);
  }
}

void properties_print(Properties *properties, FILE *fp) {
  fprintf(fp, "Properties:\n");
  for (size_t i = 0; i < properties->properties->count; i++) {
    Property *prop = (Property *) properties->properties->entries[i];
    fprintf(fp, "  ");
    property_print(prop, fp);
  }
  fprintf(fp, "\n");
}

Property *properties_find(Properties *properties, Atom name) {
  for (size_t idx = 0; idx < properties->properties->count; idx++) {
    Property *p = (Property *) properties->properties->entries[idx];   
    if (p->name == name) {
      return p;
    }
  }
  return NULL;
}

List *property_types = NULL;

void property_type_register(PropertyTypeHandler *handler) {
  if (!property_types) {
    property_types = list_create();
  }
  handler->init(handler);
  list_append(property_types, (void *) handler);
}


PropertyTypeHandler *property_type_get(Atom type, Atom name) {
  if (!property_types) return NULL;
  int best_level = -1;
  PropertyTypeHandler *best_type_handler = NULL;
  for (size_t i = 0; i < property_types->count; i++) {
    PropertyTypeHandler *type_handler = property_types->entries[i];
    if (   (type_handler->type == AnyPropertyType || type_handler->type == type)
        && (type_handler->name == AnyPropertyType || type_handler->name == name)) {
      int level = (  (type_handler->name != AnyPropertyType ? 2 : 0)
                   + (type_handler->type != AnyPropertyType ? 1 : 0));
      if (level > best_level) {
        best_type_handler = type_handler;
      }
    }
  }
  return best_type_handler;
}
