#ifndef WM
#define WM

#include "glapi.h"
#include "shader.h"
#include "space.h"

extern Shader *shader_program;
extern GLint window_sampler_attr;
extern GLint icon_sampler_attr;
extern GLint screen_attr;
extern GLint coords_attr;
extern GLint picking_mode_attr;
extern GLint window_id_attr;
extern void draw();
extern void pick(int x, int y, int *winx, int *winy, Item **item);

#endif
