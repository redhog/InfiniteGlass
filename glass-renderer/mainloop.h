#ifndef MAINLOOP_H
#define MAINLOOP_H

#include "xapi.h"
#include "list.h"
#include <sys/time.h>

typedef struct {
  List *mainloop_event_handlers;
  List *timeout_handlers;
  Bool exit_mainloop_flag;
  XConnection *conn;
} Mainloop;

#define event_mask_unset(member) memset(&member, 0x00, sizeof(member));
#define event_mask_set(member) memset(&member, 0xff, sizeof(member));

typedef struct EventHandlerStruct EventHandler;
typedef Bool EventHandlerFunction(EventHandler *handler, XEvent *event);
struct EventHandlerStruct {
  Mainloop *mainloop;
 
  long event_mask;
  XEvent match_event;
  XEvent match_mask;
 
  EventHandlerFunction *handler;
  void *data;
};

extern void mainloop_install_event_handler(EventHandler *handler);
extern void mainloop_uninstall_event_handler(EventHandler *handler);

typedef struct TimeoutHandlerStruct TimeoutHandler;
typedef void TimeoutHandlerFunction(TimeoutHandler *handler, struct timeval *current_time);
struct TimeoutHandlerStruct {
  Mainloop *mainloop;

  struct timeval next;
  struct timeval interval;
 
  TimeoutHandlerFunction *handler;
  void *data;
};

extern void mainloop_install_timeout_handler(TimeoutHandler *handler);
extern void mainloop_uninstall_timeout_handler(TimeoutHandler *handler);

extern Bool mainloop_event_handle(Mainloop *mainloop, XEvent *event);
extern void mainloop_run(Mainloop *mainloop);
extern void mainloop_exit(Mainloop *mainloop);

extern Mainloop *mainloop_create(XConnection *conn);
extern void mainloop_destroy(Mainloop *mainloop);

#endif
