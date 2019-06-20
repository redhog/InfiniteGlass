#ifndef TEXTURE
#define TEXTURE

#include "glapi.h"
#include "xapi.h"

typedef struct {
  GLXPixmap glxpixmap;
  GLuint texture_id;
} Texture;

extern void texture_from_pixmap(Texture *texture, Pixmap pixmap);
extern void texture_initialize(Texture *texture);
extern void texture_destroy(Texture *texture);

#endif
