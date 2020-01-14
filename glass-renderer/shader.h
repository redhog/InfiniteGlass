#ifndef SHADER
#define SHADER

#include "glapi.h"
#include "list.h"

typedef enum {
  UNIFORM_ATOM,
  UNIFORM_PROPERTY
} UniformName;

typedef struct {
  UniformName type;
  GLint size;
  GLenum uniform_type;
  char uniform_name[128];
  char *base_name;
} Uniform;

typedef struct {
  Atom name;
  Atom geometry;
  Atom vertex;
  Atom fragment;

  char *name_str;
 
  GLchar *geometry_src;
  GLchar *vertex_src;
  GLchar *fragment_src;
 
  GLuint program;
  GLuint geometry_shader;
  GLuint vertex_shader;
  GLuint fragment_shader;

  List *uniforms; // List of Uniform (see above)

 
  GLint screen_attr;
  GLint size_attr;
  GLint border_width_attr;

  GLint picking_mode_attr;
  GLint window_id_attr;
  GLint window_attr;
 
  GLint window_sampler_attr; 
} Shader;

extern Atom IG_SHADERS;

extern Bool init_shader();

extern Shader *shader_loadX(Atom name);
extern List *shader_load_all(void);
extern void shader_free(Shader *shader);
extern void shader_free_all(List *shaders);
extern Shader *shader_find(List *shaders, Atom name);
extern void shader_print(Shader *shader);
extern void shader_reset_uniforms(Shader *shader);

#endif
