#include "picking.h"
#include "wm.h"

GLuint picking_fb;

int init_picking() {
  GLuint color_tex;
  GLuint depth_rb;
  
  glGenTextures(1, &color_tex);
  glBindTexture(GL_TEXTURE_2D, color_tex);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, overlay_attr.width, overlay_attr.height, 0, GL_RGBA, GL_FLOAT, NULL);
  glGenFramebuffers(1, &picking_fb);
  glBindFramebuffer(GL_FRAMEBUFFER, picking_fb);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color_tex, 0);
  glGenRenderbuffers(1, &depth_rb);
  glBindRenderbuffer(GL_RENDERBUFFER, depth_rb);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, overlay_attr.width, overlay_attr.height);
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

void pick(int x, int y, int *winx, int *winy, Item **item, Item **parent_item) {
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
      current_layer = v->layers[layer_idx];
      view_draw_picking(picking_fb, v, items_all, &filter_by_layer);
    }
  }
  glFlush();
  view_pick(picking_fb, view, x, y, winx, winy, item, parent_item);
}

void raw_motion_detected(void *data, xcb_query_pointer_reply_t *reply, xcb_generic_error_t *error) {
  mouse.root = reply->root;
  mouse.win = reply->child;
  mouse.root_x = reply->root_x;
  mouse.root_y = reply->root_y;
  mouse.win_x = reply->win_x;
  mouse.win_y = reply->win_y;
  mouse.mask = reply->mask;

  int winx, winy;
  Item *item;
  Item *parent_item;

  pick(mouse.root_x, mouse.root_y, &winx, &winy, &item, &parent_item);
  if (item && item->prop_layer && item->prop_layer->values.dwords && (Atom) item->prop_layer->values.dwords[0] == ATOM("IG_LAYER_MENU")) {
    XWindowChanges values;
    values.stack_mode = Above;
    XConfigureWindow(display, item->window, CWStackMode, &values);
  } else if (item) {
    XWindowChanges values;
    values.x = mouse.root_x - winx;
    values.y = mouse.root_y - winy;
    values.stack_mode = Above;
    if (values.x != item->x || values.y != item->y) {
      XConfigureWindow(display, item->window, CWX | CWY | CWStackMode, &values);
      item->x = values.x;
      item->y = values.y;
    }

    if (parent_item && (parent_item != item->parent_item)) {
      XChangeProperty(display, item->window, ATOM("IG_PARENT_WINDOW"), XA_WINDOW, 32, PropModeReplace, (void *) &parent_item->window, 1);
      item->parent_item = parent_item;
    }

    DEBUG("position", "Point %d,%d -> %lu/%lu,%d,%d\n", mouse.root_x, mouse.root_y, parent_item ? parent_item->window : 0, item->window, winx, winy);
  } else {
    DEBUG("position", "Point %d,%d -> NONE\n", mouse.root_x, mouse.root_y);
  }
  trigger_draw();
}
