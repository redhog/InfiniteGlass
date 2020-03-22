#ifndef VIEW_TYPE
#define VIEW_TYPE

#include "xapi.h"

typedef struct {
  Atom name;
  char *name_str;
  Atom *layers;
  char **layers_str;
  size_t nr_layers;

  Atom attr_layer;
  Atom attr_view;
  Atom attr_size;

  float screen[4];
  float _screen[4];
  int width;
  int height;
  int picking;
} View;

#endif
