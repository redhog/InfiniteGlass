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

typedef struct {
 Item *item;
 View *view;
 Shader *shader;
 List *properties;
 GLuint texture_unit;
} Rendering;

#endif
