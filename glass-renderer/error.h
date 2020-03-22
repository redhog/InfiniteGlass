#ifndef ERROR_H
#define ERROR_H

#include "xapi.h"

typedef struct ErrorHandlerStruct ErrorHandler;
typedef int ErrorHandlerFunction(ErrorHandler *handler, XErrorEvent *event);

struct ErrorHandlerStruct {
  XConnection *conn;
  ErrorHandlerFunction *handler;
  void *data;
  char *context;
};

extern void error_handler_push(ErrorHandler *handler);
extern ErrorHandler *error_handler_pop(XConnection *conn);
extern void print_error(XConnection *conn, XErrorEvent *event);
extern void try(XConnection *conn);
extern int catch(XConnection *conn, XErrorEvent *error);
extern void throw(XConnection *conn, XErrorEvent *error);
extern void error_init(XConnection *conn);

#endif
