#include "rendering.h"
#include "item.h"
#include "view.h"
#include "fps.h"
#include "wm.h"

#define AUTOMATIC_REDRAWS 10

XConnection *xconn;

void draw() {
  draw_fps_start();
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glDisable(GL_SCISSOR_TEST);
  glClearColor(0., 0., 0., 1.0);
  glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
  if (views) {
    for (size_t idx = 0; idx < views->count; idx++) {
      View *v = (View *) views->entries[idx];
      for (size_t layer_idx = 0; layer_idx < v->nr_layers; layer_idx++) {
        ItemFilter filter = {
          &item_filter_by_layer,
          &v->layers[layer_idx]
        };
        view_draw(xconn, 0, v, items_all, &filter);
      }
    }
  }
  glFlush();
  glXSwapBuffers(xconn->display, xconn->overlay);
  draw_fps();
}

Bool drawn_this_cycle = False;
int draw_cycles_left = 0;

void cycle_draw() {
  if (draw_cycles_left == 0) {
    drawn_this_cycle = False;
    return;
  }
  draw();
  drawn_this_cycle = True;
  draw_cycles_left--;
}

void trigger_draw() {
  draw_cycles_left = AUTOMATIC_REDRAWS;
  if (!drawn_this_cycle) {
    draw();
    drawn_this_cycle = True;
  }
}

Bool damage_handler_function(EventHandler *handler, XEvent *event) {
  if (event->type == xconn->damage_event + XDamageNotify) {
    DEBUG("event.damage", "Received XDamageNotify: %d\n", ((XDamageNotifyEvent *) event)->drawable);
    // Subtract all the damage, repairing the window.
    Item *item = item_get_from_window(xconn, ((XDamageNotifyEvent *) event)->drawable, False);
    if (item) {
      item->draw_cycles_left = AUTOMATIC_REDRAWS;
      trigger_draw();
    }
    return True;
  }
  return False;
}

void draw_timeout_handler_function(TimeoutHandler *handler, struct timeval *current_time) {
  cycle_draw();
}

TimeoutHandler draw_timeout_handler;
EventHandler damage_event_handler;

void init_rendering(Mainloop *mainloop) {
  xconn = mainloop->conn;
 
  draw_timeout_handler.mainloop = mainloop;
  draw_timeout_handler.interval.tv_sec = 0;
  draw_timeout_handler.interval.tv_usec = 30000;
  if (gettimeofday(&draw_timeout_handler.next, NULL) != 0) {
    ERROR("gettimeofday", "gettimeofday() returned error");
  }
  draw_timeout_handler.handler = &draw_timeout_handler_function;
  draw_timeout_handler.data = NULL;
  mainloop_install_timeout_handler(&draw_timeout_handler);

  damage_event_handler.mainloop = mainloop;
  damage_event_handler.event_mask = SubstructureRedirectMask;
  event_mask_unset(damage_event_handler.match_event);
  event_mask_unset(damage_event_handler.match_mask);
  damage_event_handler.match_event.type = xconn->damage_event + XDamageNotify;
  event_mask_set(damage_event_handler.match_mask.type);
  damage_event_handler.handler = &damage_handler_function;
  damage_event_handler.data = NULL;
  mainloop_install_event_handler(&damage_event_handler);

  trigger_draw();
}
