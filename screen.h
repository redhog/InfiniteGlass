#ifndef SCREEN
#define SCREEN

extern float screen[4];
extern void mat4mul(float *mat4, float *vec4, float *outvec4);
extern void screen2space(float screenx, float screeny, float *spacex, float *spacey);
extern void space2screen(float spacex, float spacey, float *screenx, float *screeny);

#endif

