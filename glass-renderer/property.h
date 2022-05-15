#ifndef PROPERTY
#define PROPERTY

#include "xapi.h"
#include "glapi.h"
#include "list.h"
#include "rendering.h"
#include "shader.h"
#include <stdio.h>

#define PROGRAM_CACHE_SIZE 10

struct ProgramCacheStruct;
typedef struct ProgramCacheStruct ProgramCache;

struct PropertiesStruct;
typedef struct PropertiesStruct Properties;

struct PropertyProgramCacheStruct;
typedef struct PropertyProgramCacheStruct PropertyProgramCache;

struct PropertyStruct;
typedef struct PropertyStruct Property;

struct PropertyTypeHandlerT;
typedef struct PropertyTypeHandlerT PropertyTypeHandler;


struct PropertyProgramCacheStruct {
  GLint program;
  Shader *shader;
  char *prefix;
  char *name_str;
  Bool is_uniform;
  GLint location;
  GLuint buffer;
  GLint size;
  GLenum type;
  void *data;
};
struct PropertyStruct {
  Window window;
  Atom name;
  char *name_str;
  Atom type;
  int format;
  unsigned long nitems;
  union {
    uint8_t *bytes;
    uint16_t *words;
    uint32_t *dwords;
  } values;
  PropertyProgramCache programs[PROGRAM_CACHE_SIZE];
  char *data;
  PropertyTypeHandler *type_handler;
  xcb_get_property_reply_t *property_get_reply;
};

extern Property *property_allocate(Properties *properties, Atom name);
extern void property_load(Property *prop);
extern void property_free(Property *prop);
extern void property_to_gl(Property *prop, Rendering *rendering);
extern void property_draw(Property *prop, Rendering *rendering);
extern void property_print(Property *prop, int indent, FILE *fp);

struct ProgramCacheStruct {
  GLint program;
  Shader *shader;
  char *prefix;
};
struct PropertiesStruct {
  Window window;
  List *properties;
  ProgramCache programs[PROGRAM_CACHE_SIZE];
  size_t programs_pos;
};

extern Properties *properties_load(Window window);
extern void properties_update(Properties *properties, Atom name);
extern void properties_free(Properties *properties);
extern void properties_to_gl(Properties *properties, char *prefix, Rendering *rendering);
extern void properties_draw(Properties *properties, Rendering *rendering);
extern void properties_print(Properties *properties, int indent, FILE *fp);
extern Property *properties_find(Properties *properties, Atom name);

typedef void PropertyInit(PropertyTypeHandler *prop);
typedef int  PropertyMatch(Property *prop);
typedef void PropertyLoad(Property *prop);
typedef void PropertyFree(Property *prop);
typedef void PropertyLoadProgram(Property *prop, Rendering *rendering);
typedef void PropertyFreeProgram(Property *prop, size_t index);
typedef void PropertyToGl(Property *prop, Rendering *rendering);
typedef void PropertyCalculate(Property *prop, Rendering *rendering);
typedef void PropertyDraw(Property *prop, Rendering *rendering);
typedef void PropertyPrint(Property *prop, int indent, FILE *fp);

struct PropertyTypeHandlerT {
  PropertyInit *init;
  PropertyMatch *match;
  PropertyLoad *load;
  PropertyFree *free;
  PropertyToGl *to_gl;
  PropertyPrint *print;
  PropertyLoadProgram *load_program;
  PropertyFreeProgram *free_program;
  PropertyDraw *draw;
  PropertyCalculate *calculate;
 
  Atom type;
  Atom name;
};

extern void property_type_register(PropertyTypeHandler *handler);
extern PropertyTypeHandler *property_type_get(Property *prop);


#endif
