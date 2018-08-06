#ifndef WM
#define WM

#include "glapi.h"
#include "shader.h"

extern Shader *shader_program;
extern GLint sampler_attr;
extern GLint screen_attr;
extern unsigned int coords_attr;
extern float screen[4];
extern void draw();

#endif
