#ifndef SHADER
#define SHADER

#include "glapi.h"
#include "list.h"

typedef struct {
  Atom name;
  Atom geometry;
  Atom vertex;
  Atom fragment;

  GLchar *geometry_src;
  GLchar *vertex_src;
  GLchar *fragment_src;
 
  GLuint program;
  GLuint geometry_shader;
  GLuint vertex_shader;
  GLuint fragment_shader;
} Shader;

extern Shader *shader_loadX(Atom name);
extern List *shader_load_all(void);
extern void shader_free(Shader *shader);
extern void shader_free_all(List *shaders);
extern Shader *shader_find(List *shaders, Atom name);
extern void shader_print(Shader *shader);

#endif
