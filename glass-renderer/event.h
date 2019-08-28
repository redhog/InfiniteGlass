#ifndef EVENT
#define EVENT

#include "xapi.h"

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

extern Bool event_handle(XEvent *event);
extern void event_handler_install(EventHandler *handler);
extern void event_handler_uninstall(EventHandler *handler);

#endif
