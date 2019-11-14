#ifndef TEXTURE
#define TEXTURE

#include "glapi.h"
#include "xapi.h"
#include <cairo.h>

typedef struct {
  GLXPixmap glxpixmap;
  GLuint texture_id;
} Texture;

extern void texture_from_icon(Texture *texture, unsigned long *data);
extern void texture_from_cairo_surface(Texture *texture, cairo_surface_t *surface);
extern void texture_from_pixmap(Texture *texture, Pixmap pixmap);
extern void texture_initialize(Texture *texture);
extern void texture_destroy(Texture *texture);

#endif
