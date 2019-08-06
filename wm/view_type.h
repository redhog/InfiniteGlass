#ifndef VIEW_TYPE
#define VIEW_TYPE

#include "xapi.h"

typedef struct {
  Atom name;
  Atom layer;

  Atom attr_layer;
  Atom attr_view;

  float screen[4];
  int width;
  int height;
  int picking;
} View;

#endif
