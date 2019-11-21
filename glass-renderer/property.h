#ifndef PROPERTY
#define PROPERTY

#include "xapi.h"
#include "glapi.h"
#include "list.h"
#include "rendering.h"
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



struct PropertyProgramCacheStruct {
  GLint program; 
  char *name_str;
  GLint location;
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
    unsigned char *bytes;
    unsigned short *words;
    unsigned long *dwords;
  } values;
  PropertyProgramCache programs[PROGRAM_CACHE_SIZE];
  char *data;
};

extern Property *property_allocate(Properties *properties, Atom name);
extern Bool property_load(Property *prop);
extern void property_free(Property *prop);
extern void property_to_gl(Property *prop, Rendering *rendering);
extern void property_print(Property *prop, FILE *fp);

struct ProgramCacheStruct {
  GLint program;
  char *prefix;
};
struct PropertiesStruct {
  Window window;
  List *properties;
  ProgramCache programs[PROGRAM_CACHE_SIZE];
  size_t programs_pos;
};

extern Properties *properties_load(Window window);
extern Bool properties_update(Properties *properties, Atom name);
extern void properties_free(Properties *properties);
extern void properties_to_gl(Properties *properties, char *prefix, Rendering *rendering);
extern void properties_print(Properties *properties, FILE *fp);
extern Property *properties_find(Properties *properties, Atom name);

struct PropertyTypeHandlerT;
typedef struct PropertyTypeHandlerT PropertyTypeHandler;

typedef void PropertyInit(PropertyTypeHandler *prop);
typedef void PropertyLoad(Property *prop);
typedef void PropertyFree(Property *prop);
typedef void PropertyLoadProgram(Property *prop, Rendering *rendering);
typedef void PropertyFreeProgram(Property *prop, size_t index);
typedef void PropertyToGl(Property *prop, Rendering *rendering);
typedef void PropertyPrint(Property *prop, FILE *fp);

struct PropertyTypeHandlerT {
  PropertyInit *init;
  PropertyLoad *load;
  PropertyFree *free;
  PropertyToGl *to_gl;
  PropertyPrint *print;
  PropertyLoadProgram *load_program;
  PropertyFreeProgram *free_program;
 
  Atom type;
  Atom name;
};

extern void property_type_register(PropertyTypeHandler *handler);
extern PropertyTypeHandler *property_type_get(Atom type, Atom name);


#endif
