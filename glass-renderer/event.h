#ifndef EVENT
#define EVENT

#include "xapi.h"
#include <sys/time.h>

#define event_mask_unset(member) memset(&member, 0x00, sizeof(member));
#define event_mask_set(member) memset(&member, 0xff, sizeof(member));

typedef struct EventHandlerStruct EventHandler;
typedef Bool EventHandlerFunction(EventHandler *handler, XEvent *event);
struct EventHandlerStruct {
  long event_mask;
  XEvent match_event;
  XEvent match_mask;
 
  EventHandlerFunction *handler;
  void *data;
};

extern void event_handler_install(EventHandler *handler);
extern void event_handler_uninstall(EventHandler *handler);

typedef struct TimeoutHandlerStruct TimeoutHandler;
typedef void TimeoutHandlerFunction(TimeoutHandler *handler, struct timeval *current_time);
struct TimeoutHandlerStruct {
  struct timeval next;
  struct timeval interval;
 
  TimeoutHandlerFunction *handler;
  void *data;
};

extern void timeout_handler_install(TimeoutHandler *handler);
extern void timeout_handler_uninstall(TimeoutHandler *handler);

extern Bool event_handle(XEvent *event);
extern void event_mainloop();
extern void event_exit_mainloop();


#endif
