#ifndef RENDERING
#define RENDERING

#include "view_type.h"
#include "shader.h"
#include "list.h"
#include "glapi.h"
#include "xapi.h"
#include "mainloop.h"

struct PropertyStruct;
typedef struct PropertyStruct Property;

struct ItemStruct;
typedef struct ItemStruct Item;

struct PropertiesStruct;
typedef struct PropertiesStruct Properties;

typedef struct {
 XConnection *conn;
 Item *item; // The current item being rendered
 Item *parent_item; // The parent item that we're rendering this item for
 Item *source_item; // The item that owns the properties being converted to uniforms. Can be e.g same as item or parent_item or root_item...
 int widget_id; // The property index on the parent item that caused this item to be rendered
 View *view;
 Shader *shader;
 Properties *properties;
 char *properties_prefix;
 size_t program_cache_idx;
 GLuint texture_unit;
 size_t array_length;
} Rendering;

void trigger_draw();
void init_rendering(Mainloop *mainloop);

#endif
