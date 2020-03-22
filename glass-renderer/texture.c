#include "texture.h"

void texture_from_icon(Texture *texture, unsigned long *data) {
  // Format used by _NET_WM_ICON
  // This is an array of 32bit packed CARDINAL ARGB with high byte being A, low byte being B.
  // The first two cardinals are width, height. Data is in rows, left to right and top to bottom.
  // Note: As usual with window properties in Xlib, 32 bits are delivered to the application as
  // an unsigned long, even on platforms where the size of unsigned long is 64 bits...
 
  unsigned long width = data[0];
  unsigned long height = data[1];
  unsigned char *tmp = malloc(width * height * 4);
  for (size_t i = 0; i < width * height; i++) {
    unsigned char *color = (unsigned char *) &(data[2 + i]);
    tmp[i * 4 + 0] = color[2]; //R
    tmp[i * 4 + 1] = color[1]; //G 
    tmp[i * 4 + 2] = color[0]; //B
    tmp[i * 4 + 3] = color[3]; //A
  }

  if (!texture->texture_id) {
    glGenTextures(1, &texture->texture_id);
    GL_CHECK_ERROR("texture_from_icon", "");
  }
  glBindTexture(GL_TEXTURE_2D, texture->texture_id);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, tmp);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  free(tmp);
  GL_CHECK_ERROR("texture_from_icon", "");
}

void texture_from_cairo_surface(Texture *texture, cairo_surface_t *surface) {
  if (!texture->texture_id) {
    glGenTextures(1, &texture->texture_id);
    GL_CHECK_ERROR("texture_from_cairo_surface1", "");
  }

  glBindTexture(GL_TEXTURE_2D, texture->texture_id);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_BLUE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_GREEN);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_RED);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_ALPHA);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
               cairo_image_surface_get_width(surface),
               cairo_image_surface_get_height(surface),
               0, GL_RGBA, GL_UNSIGNED_BYTE,
               cairo_image_surface_get_data(surface));
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  GL_CHECK_ERROR("texture_from_cairo_surface2", "");
}

void texture_from_glpixmap(XConnection *conn, Texture *texture) {
  if (!texture->texture_id) {
    glGenTextures(1, &texture->texture_id);
    GL_CHECK_ERROR("texture_from_pixmap1", "");
  }
  glBindTexture(GL_TEXTURE_2D, texture->texture_id);
  GL_CHECK_ERROR("texture_from_pixmap2", "");
  conn->glXBindTexImageEXT(conn->display, texture->glxpixmap, GLX_FRONT_EXT, NULL);
  GL_CHECK_ERROR("texture_from_pixmap3", "");
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  GL_CHECK_ERROR("texture_from_pixmap4", "");
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  GL_CHECK_ERROR("texture_from_pixmap5", "");
}

void texture_from_pixmap(XConnection *conn, Texture *texture, Pixmap pixmap) {
  if (texture->glxpixmap) {
    glXDestroyGLXPixmap(conn->display, texture->glxpixmap);
    texture->glxpixmap = 0;
  }
  
  const int pixmap_attribs[] = {
    GLX_TEXTURE_TARGET_EXT, GLX_TEXTURE_2D_EXT,
    GLX_TEXTURE_FORMAT_EXT, GLX_TEXTURE_FORMAT_RGB_EXT,
    None
  };
  GL_CHECK_ERROR("texture_from_pixmap1", "");

  // Make sure to generate a BadDrawable here if the pixmap is broken,
  // as glXBindTexImageEXT will SIGSEGV if that happens...!
  Window root_return;
  int x_return;
  int y_return;
  unsigned int width_return;
  unsigned int height_return;
  unsigned int border_width_return;
  unsigned int depth_return;
  if (!XGetGeometry(conn->display, pixmap, &root_return,
                    &x_return, &y_return, &width_return, &height_return, &border_width_return, &depth_return)) {
    return;
  }
  
  texture->glxpixmap = glXCreatePixmap(conn->display, configs[0], pixmap, pixmap_attribs);

  texture_from_glpixmap(conn, texture);
}

void texture_initialize(Texture *texture) {
  texture->glxpixmap = 0;
  texture->texture_id = 0;
}

void texture_destroy(XConnection *conn, Texture *texture) {
  if (texture->glxpixmap) {
    glXDestroyGLXPixmap(conn->display, texture->glxpixmap);
    texture->glxpixmap = 0;
  }
  if (texture->texture_id) {
    glDeleteTextures(1, &texture->texture_id);
    texture->texture_id = 0;
  }
}
