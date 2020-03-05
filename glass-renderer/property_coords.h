#ifndef PROPERTY_COORDS
#define PROPERTY_COORDS

#include "property.h"


typedef struct {
  float *coords;
  float ccoords[4];
} PropertyCoords;

extern PropertyTypeHandler property_coords;


#endif
