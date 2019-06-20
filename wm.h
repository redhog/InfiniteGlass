#ifndef WM
#define WM

#include "glapi.h"
#include "shader.h"
#include "item.h"
#include "item_window.h"

extern Shader *shader_program;
extern GLint window_sampler_attr;
extern GLint icon_sampler_attr;
extern GLint icon_mask_sampler_attr;
extern GLint screen_attr;
extern GLint coords_attr;
extern GLint picking_mode_attr;
extern GLint has_icon_attr;
extern GLint has_icon_mask_attr;
extern GLint window_id_attr;
extern void draw();
extern void pick(int x, int y, int *winx, int *winy, Item **item);

#endif
