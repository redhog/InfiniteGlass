#include "picking.h"
#include "view.h"

GLuint picking_fb;

void pick(XConnection *conn, int x, int y, int *winx, int *winy, Item **item, Item **parent_item) {
  if (!views) {
    *winy = *winx = 0;
    *item = NULL;
    return;
  }
  View *view = (View *) views->entries[0];
  GL_CHECK_ERROR("pick1", "");
  glBindFramebuffer(GL_FRAMEBUFFER, picking_fb);
  glEnable(GL_SCISSOR_TEST);
  glScissor(x, view->height - y, 1, 1);
  glClearColor(0.0, 0.0, 0.0, 1.0);
  glClear(GL_COLOR_BUFFER_BIT);
  for (size_t idx = 0; idx < views->count; idx++) {
    View *v = (View *) views->entries[idx];
    for (size_t layer_idx = 0; layer_idx < v->nr_layers; layer_idx++) {
      ItemFilter filter = {
        &item_filter_by_layer,
        &v->layers[layer_idx]
      };
      view_draw_picking(conn, picking_fb, v, items_all, &filter);
    }
  }
  glFlush();
  view_pick(conn, picking_fb, view, x, y, winx, winy, item, parent_item);
}

int init_picking(XConnection *conn) {
  GLuint color_tex;
  GLuint depth_rb;
  
  glGenTextures(1, &color_tex);
  glBindTexture(GL_TEXTURE_2D, color_tex);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, conn->overlay_attr.width, conn->overlay_attr.height, 0, GL_RGBA, GL_FLOAT, NULL);
  glGenFramebuffers(1, &picking_fb);
  glBindFramebuffer(GL_FRAMEBUFFER, picking_fb);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color_tex, 0);
  glGenRenderbuffers(1, &depth_rb);
  glBindRenderbuffer(GL_RENDERBUFFER, depth_rb);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, conn->overlay_attr.width, conn->overlay_attr.height);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_rb);
  GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  switch(status) {
    case GL_FRAMEBUFFER_COMPLETE:
      break;
    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
      ERROR("init_picking", "Not all framebuffer attachment points are framebuffer attachment complete.\n");
      return 0;
    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
      ERROR("init_picking", "No images are attached to the framebuffer.\n");
      return 0;
    case GL_FRAMEBUFFER_UNSUPPORTED:
      ERROR("init_picking", "The combination of internal formats of the attached images violates an implementation-dependent set of restrictions.\n");
      return 0;
    default:
      ERROR("init_picking", "Unknown error.\n");
      return 0;
  }
  return 1;
}
