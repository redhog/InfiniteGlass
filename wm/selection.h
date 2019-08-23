#ifndef SELECTION
#define SELECTION

#include "xapi.h"
#include "event.h"

/* This module implements selection handling according to
   https://www.x.org/releases/X11R7.6/doc/xorg-docs/specs/ICCCM/icccm.html#acquiring_selection_ownership
   https://www.x.org/releases/X11R7.6/doc/xorg-docs/specs/ICCCM/icccm.html#responsibilities_of_the_selection_owner

   In addition, it provides an implementation of manager selections in accordance with

   https://www.x.org/releases/X11R7.6/doc/xorg-docs/specs/ICCCM/icccm.html#manager_selections
*/

typedef struct SelectionStruct Selection;
typedef Bool SelectionHandler(Selection *selection, XEvent *event);

struct SelectionStruct {
 SelectionHandler *handler;
 void *data;
 Window owner;
 Atom name;
 Time time;

 EventHandler event_handler_convert;
};

extern int selection_get_params(Selection *selection, XEvent *event, long offset, long length,
                                Atom *actual_type_return, int *actual_format_return,
                                unsigned long *nitems_return, unsigned long *bytes_after_return, unsigned char **prop_return);
extern void selection_answer(Selection *selection, XEvent *event, Atom type, int format, int mode, unsignedchar *data, int nelements);
extern Select *selection_create(Window owner, Atom name, SelectionHandler *handler, void *data);

#endif
