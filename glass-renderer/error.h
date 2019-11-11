#ifndef ERROR_H
#define ERROR_H

#include "xapi.h"

typedef struct ErrorHandlerStruct ErrorHandler;
typedef int ErrorHandlerFunction(ErrorHandler *handler, XErrorEvent *event);

struct ErrorHandlerStruct {
  ErrorHandlerFunction *handler;
  void *data;
  char *context;
};

extern void error_handler_push(ErrorHandler *handler);
extern ErrorHandler *error_handler_pop();
extern void print_error(XErrorEvent *event);
extern void try();
extern int catch(XErrorEvent *error);
extern void throw(XErrorEvent *error);
extern void error_init();

#endif
