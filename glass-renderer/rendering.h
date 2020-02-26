#ifndef RENDERING
#define RENDERING

#include "view_type.h"
#include "shader.h"
#include "list.h"
#include "glapi.h"

struct PropertyStruct;
typedef struct PropertyStruct Property;

struct ItemStruct;
typedef struct ItemStruct Item;

struct PropertiesStruct;
typedef struct PropertiesStruct Properties;

typedef struct {
 Item *item;
 Item *parent_item;
 View *view;
 float screen[4]; // Is different from view->screen when rendering sub-items
 Shader *shader;
 Properties *properties;
 char *properties_prefix;
 size_t program_cache_idx;
 GLuint texture_unit;
 size_t array_length;
} Rendering;

#endif
