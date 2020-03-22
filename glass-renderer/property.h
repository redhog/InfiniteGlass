#ifndef PROPERTY
#define PROPERTY

#include "xapi.h"
#include "glapi.h"
#include "list.h"
#include "rendering.h"
#include "shader.h"
#include <stdio.h>

#define PROGRAM_CACHE_SIZE 2

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
  char *type_str;
  int format;
  unsigned long nitems;
  union {
    unsigned char *bytes;
    unsigned short *words;
    unsigned long *dwords;
  } values;
  PropertyProgramCache programs[PROGRAM_CACHE_SIZE];
  char *data;
  PropertyTypeHandler *type_handler;
};

extern Property *property_allocate(XConnection *conn, Properties *properties, Atom name);
extern Bool property_load(XConnection *conn, Property *prop);
extern void property_free(XConnection *conn, Property *prop);
extern void property_to_gl(Property *prop, Rendering *rendering);
extern void property_draw(Property *prop, Rendering *rendering);
extern void property_print(XConnection *conn, Property *prop, FILE *fp);

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

extern Properties *properties_load(XConnection *conn, Window window);
extern Bool properties_update(XConnection *conn, Properties *properties, Atom name);
extern void properties_free(XConnection *conn, Properties *properties);
extern void properties_to_gl(Properties *properties, char *prefix, Rendering *rendering);
extern void properties_draw(Properties *properties, Rendering *rendering);
extern void properties_print(XConnection *conn, Properties *properties, FILE *fp);
extern Property *properties_find(Properties *properties, Atom name);

typedef void PropertyInit(XConnection *conn, PropertyTypeHandler *prop);
typedef void PropertyLoad(XConnection *conn, Property *prop);
typedef void PropertyFree(XConnection *conn, Property *prop);
typedef void PropertyLoadProgram(Property *prop, Rendering *rendering);
typedef void PropertyFreeProgram(Property *prop, size_t index);
typedef void PropertyToGl(Property *prop, Rendering *rendering);
typedef void PropertyCalculate(Property *prop, Rendering *rendering);
typedef void PropertyDraw(Property *prop, Rendering *rendering);
typedef void PropertyPrint(XConnection *conn, Property *prop, FILE *fp);

struct PropertyTypeHandlerT {
  PropertyInit *init;
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

extern void property_type_register(XConnection *conn, PropertyTypeHandler *handler);
extern PropertyTypeHandler *property_type_get(Atom type, Atom name);


#endif
