#include "mainloop.h"
#include "list.h"
#include "debug.h"
#include <string.h>
#include <poll.h>

List *mainloop_fd_handlers = NULL;
struct pollfd *mainloop_fds = NULL;
List *timeout_handlers = NULL;
List *mainloop_displays = NULL;


void mainloop_update_fds() {
  mainloop_fds = realloc(mainloop_fds, sizeof(struct pollfd) * mainloop_fd_handlers->count);
  for (size_t idx = 0; idx < mainloop_fd_handlers->count; idx++) {
    FdEventHandler *handler = (FdEventHandler *) mainloop_fd_handlers->entries[idx];
    mainloop_fds[idx].fd = handler->fd;
    mainloop_fds[idx].events = handler->events;
  }
}

void mainloop_install_fd_event_handler(FdEventHandler *handler) {
  if (!mainloop_fd_handlers) mainloop_fd_handlers = list_create();
  list_append(mainloop_fd_handlers, (void *) handler);
  mainloop_update_fds();
}
void mainloop_uninstall_fd_event_handler(FdEventHandler *handler) {
  list_remove(mainloop_fd_handlers, (void *) handler);
  mainloop_update_fds();
}


void mainloop_install_timeout_handler(TimeoutHandler *handler) {
  if (!timeout_handlers) timeout_handlers = list_create();
  list_append(timeout_handlers, (void *) handler);
}

void mainloop_uninstall_timeout_handler(TimeoutHandler *handler) {
  list_remove(timeout_handlers, (void *) handler);
}

Bool event_match(XEvent *event, XEvent *match_event, XEvent *match_mask) {
 char *e = (char *) event;
 char *me = (char *) match_event;
 char *mm = (char *) match_mask;

  for (size_t idx = 0; idx < sizeof(XEvent); idx++) {
    if ((e[idx] & mm[idx]) != (me[idx] & mm[idx])) {
      return False;
    }
  }
  return True;
}

void mainloop_display_xevent_handler(DisplayHandler *handler, XEvent *event) {
  for (int idx = 0; idx < handler->event_handlers->count; idx++) {
    XEventHandler *xevent_handler = (XEventHandler *) handler->event_handlers->entries[idx];
    if (   event_match(event, &xevent_handler->match_event, &xevent_handler->match_mask)
        && xevent_handler->handler(xevent_handler, event)) {
      return;
    }
  }
}

void mainloop_display_handler(FdEventHandler *handler, short revents) {
  XEvent xevent;
  DisplayHandler *display_handler = (DisplayHandler *) handler->data;
  Display *display = display_handler->display;
  while (XPending(display)) {
    XNextEvent(display, &xevent);
    mainloop_display_xevent_handler(display_handler, &xevent);
    XSync(display, False);
  }
}

DisplayHandler *mainloop_install_display(Display *display) {
  DisplayHandler *handler = (DisplayHandler *) malloc(sizeof(DisplayHandler));
  handler->display = display;
  handler->event_handlers = list_create();
  handler->fd_handler.fd = ConnectionNumber(display);
  handler->fd_handler.events = POLLIN;
  handler->fd_handler.handler = &mainloop_display_handler;
  handler->fd_handler.data = (void *) handler;
  
  if (!mainloop_displays) mainloop_displays = list_create();
  list_append(mainloop_displays, (void *) handler);
  return handler;
}
void mainloop_uninstall_display(DisplayHandler *handler) {
  list_remove(mainloop_displays, (void *) handler);
  list_destroy(handler->event_handlers);
  free(handler);
}

void mainloop_install_xevent_handler(XEventHandler *handler) {
  list_append(handler->display->event_handlers, (void *) handler);
  // Fetch existing mask and OR!!!
  if (handler->match_mask.xany.window) {
    XSelectInput(handler->display->display, handler->match_event.xany.window, handler->event_mask);
  }
}

void mainloop_uninstall_xevent_handler(XEventHandler *handler) {
  list_remove(handler->display->event_handlers, (void *) handler);
}

void timeout_handle() {
  if (!timeout_handlers) return;
  struct timeval current_time;
  if (gettimeofday(&current_time, NULL) != 0) {
    ERROR("gettimeofday", "gettimeofday() returned error");
  }
  for (int idx = 0; idx < timeout_handlers->count; idx++) {
    TimeoutHandler *handler = (TimeoutHandler *) timeout_handlers->entries[idx];
    if (timerisset(&handler->next) && timercmp(&current_time, &handler->next, >=)) {
      handler->handler(handler, &current_time);
      if (timerisset(&handler->interval)) {
        struct timeval res;
        while (timercmp(&current_time, &handler->next, >=)) {
          timeradd(&handler->next, &handler->interval, &res);
          handler->next = res;
        }
      } else {
        timerclear(&handler->next);
      }
    }
  }
  return;
}

Bool exit_mainloop_flag = False;

void mainloop_run() {
  int res;
  while (!exit_mainloop_flag) {
    res = poll(mainloop_fds, mainloop_fd_handlers->count, 30);
    timeout_handle();
    if (res > 0) {
      for (size_t idx = 0; idx < mainloop_fd_handlers->count; idx++) {
        if (mainloop_fds[idx].revents) {
          FdEventHandler *handler = (FdEventHandler *) mainloop_fd_handlers->entries[idx];
          handler->handler(handler, mainloop_fds[idx].revents);
        }
      }     
    }
  }
}

void mainloop_exit() {
  exit_mainloop_flag = True;
}
