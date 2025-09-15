#ifndef PROPERTY_COORDS
#define PROPERTY_COORDS

#include "property.h"


typedef struct {
  float *coords; // Value from IG_COORDS
  float ccoords[4]; // Calculated coords from props specified in IG_COORD_TYPES
} PropertyCoords;

extern PropertyTypeHandler property_coords;


#endif
