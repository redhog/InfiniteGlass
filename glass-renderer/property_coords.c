#include "property_coords.h"
#include "shader.h"
#include "rendering.h"
#include "item.h"
#include "debug.h"
#include <math.h>

Atom IG_COORD_DESKTOP = None;
Atom IG_COORD_PARENT_BASE = None;
Atom IG_COORD_PARENT = None;
Atom IG_COORD_PARENT_X = None;
Atom IG_COORD_PARENT_Y = None;
Atom IG_COORD_SCREEN_BASE = None;
Atom IG_COORD_SCREEN = None;
Atom IG_COORD_SCREEN_X = None;
Atom IG_COORD_SCREEN_Y = None;

#define FL(value) *((float *) &value)
void property_coords_init(PropertyTypeHandler *prop) {
  prop->type = XA_FLOAT;
  prop->name = IG_COORDS;

  if (IG_COORD_DESKTOP == None) IG_COORD_DESKTOP = XInternAtom(display, "IG_COORD_DESKTOP", False);
  if (IG_COORD_PARENT_BASE == None) IG_COORD_PARENT_BASE = XInternAtom(display, "IG_COORD_PARENT_BASE", False);
  if (IG_COORD_PARENT == None) IG_COORD_PARENT = XInternAtom(display, "IG_COORD_PARENT", False);
  if (IG_COORD_PARENT_X == None) IG_COORD_PARENT = XInternAtom(display, "IG_COORD_PARENT_X", False);
  if (IG_COORD_PARENT_Y == None) IG_COORD_PARENT = XInternAtom(display, "IG_COORD_PARENT_Y", False);
  if (IG_COORD_SCREEN_BASE == None) IG_COORD_SCREEN_BASE = XInternAtom(display, "IG_COORD_SCREEN_BASE", False);
  if (IG_COORD_SCREEN == None) IG_COORD_SCREEN = XInternAtom(display, "IG_COORD_SCREEN", False);
  if (IG_COORD_SCREEN_X == None) IG_COORD_SCREEN_X = XInternAtom(display, "IG_COORD_SCREEN_X", False);
  if (IG_COORD_SCREEN_Y == None) IG_COORD_SCREEN_Y = XInternAtom(display, "IG_COORD_SCREEN_Y", False);
}

void property_coords_load(Property *prop) {
  PropertyCoords *data = (PropertyCoords *) prop->data;
  if (!data) {
    prop->data = malloc(sizeof(PropertyCoords));
    data = (PropertyCoords *) prop->data;
    data->coords = NULL;
  }
  data->coords = realloc(data->coords, sizeof(float) * prop->nitems);
  for (int i = 0; i < prop->nitems; i++) {
    data->coords[i] = FL(prop->values.dwords[i]);
  }
}

void property_coords_free(Property *prop) {
  if (prop->data) {
    PropertyCoords *data = (PropertyCoords *) prop->data;
    if (data->coords) free(data->coords);
    free(data);
  }
}

void property_coords_calculate(Property *prop, Rendering *rendering) {
  PropertyProgramCache *prop_cache = &prop->programs[rendering->program_cache_idx];
  if (prop_cache->location == -1) return;
  if (!prop_cache->is_uniform) return;
  if (prop->nitems < 4 || prop->nitems % 4 != 0) return;
  if (prop_cache->type != GL_FLOAT_VEC4) return;
  
  PropertyCoords *data = (PropertyCoords *) prop->data;
  PropertyCoords *parent_data = NULL;
  if (rendering->parent_item && rendering->parent_item->prop_coords) {
    parent_data = (PropertyCoords *) rendering->parent_item->prop_coords->data;
  }
  
  data->ccoords[0] = 0.0;
  data->ccoords[1] = 0.0;
  data->ccoords[2] = 0.0;
  data->ccoords[3] = 0.0;

  int types_nitems = 0;
  Atom *types = NULL;
  if (rendering->item->prop_coord_types) {
    types_nitems = rendering->item->prop_coord_types->nitems;
    types = (Atom *) rendering->item->prop_coord_types->values.dwords;
  }

  for (int i = 0; i < prop->nitems; i += 4) {
    Atom type = IG_COORD_DESKTOP;
    if (i / 4 < types_nitems) type = types[i / 4];

    if (type == IG_COORD_DESKTOP) {
      data->ccoords[0] += data->coords[i+0];
      data->ccoords[1] += data->coords[i+1];
      data->ccoords[2] += data->coords[i+2];
      data->ccoords[3] += data->coords[i+3];
    } else if (type == IG_COORD_PARENT_BASE && parent_data) {
      data->ccoords[0] += parent_data->ccoords[0] + data->coords[i+0] * parent_data->ccoords[2];
      data->ccoords[1] += parent_data->ccoords[1] + data->coords[i+1] * parent_data->ccoords[3];
      data->ccoords[2] += data->coords[i+2] * parent_data->ccoords[2];
      data->ccoords[3] += data->coords[i+3] * parent_data->ccoords[3];
    } else if (type == IG_COORD_PARENT && parent_data) {
      data->ccoords[0] += data->coords[i+0] * parent_data->ccoords[2];
      data->ccoords[1] += data->coords[i+1] * parent_data->ccoords[3];
      data->ccoords[2] += data->coords[i+2] * parent_data->ccoords[2];
      data->ccoords[3] += data->coords[i+3] * parent_data->ccoords[3];
    } else if (type == IG_COORD_PARENT_X && parent_data) {
      data->ccoords[0] += data->coords[i+0] * parent_data->ccoords[2];
      data->ccoords[1] += data->coords[i+1] * parent_data->ccoords[2];
      data->ccoords[2] += data->coords[i+2] * parent_data->ccoords[2];
      data->ccoords[3] += data->coords[i+3] * parent_data->ccoords[2];
    } else if (type == IG_COORD_PARENT_Y && parent_data) {
      data->ccoords[0] += data->coords[i+0] * parent_data->ccoords[3];
      data->ccoords[1] += data->coords[i+1] * parent_data->ccoords[3];
      data->ccoords[2] += data->coords[i+2] * parent_data->ccoords[3];
      data->ccoords[3] += data->coords[i+3] * parent_data->ccoords[3];
    } else if (type == IG_COORD_SCREEN_BASE) {
      data->ccoords[0] += rendering->view->screen[0] + data->coords[i+0] * rendering->view->screen[2];
      data->ccoords[1] += rendering->view->screen[1] + data->coords[i+1] * rendering->view->screen[3];
      data->ccoords[2] += data->coords[i+2] * rendering->view->screen[2];
      data->ccoords[3] += data->coords[i+3] * rendering->view->screen[3];
    } else if (type == IG_COORD_SCREEN) {
      data->ccoords[0] += data->coords[i+0] * rendering->view->screen[2];
      data->ccoords[1] += data->coords[i+1] * rendering->view->screen[3];
      data->ccoords[2] += data->coords[i+2] * rendering->view->screen[2];
      data->ccoords[3] += data->coords[i+3] * rendering->view->screen[3];
    } else if (type == IG_COORD_SCREEN_X) {
      data->ccoords[0] += data->coords[i+0] * rendering->view->screen[2];
      data->ccoords[1] += data->coords[i+1] * rendering->view->screen[2];
      data->ccoords[2] += data->coords[i+2] * rendering->view->screen[2];
      data->ccoords[3] += data->coords[i+3] * rendering->view->screen[2];
    } else if (type == IG_COORD_SCREEN_Y) {
      data->ccoords[0] += data->coords[i+0] * rendering->view->screen[3];
      data->ccoords[1] += data->coords[i+1] * rendering->view->screen[3];
      data->ccoords[2] += data->coords[i+2] * rendering->view->screen[3];
      data->ccoords[3] += data->coords[i+3] * rendering->view->screen[3];
    }
  }
}

void property_coords_to_gl(Property *prop, Rendering *rendering) {
  PropertyProgramCache *prop_cache = &prop->programs[rendering->program_cache_idx];
  PropertyCoords *data = (PropertyCoords *) prop->data;
  glUniform4f(prop_cache->location, data->ccoords[0], data->ccoords[1], data->ccoords[2], data->ccoords[3]);
}

void property_coords_print(Property *prop, FILE *fp) {
  float *values = (float *) prop->data;
  fprintf(fp, "%ld.%s=<coords>", prop->window, prop->name_str);
  for (int i = 0; i <prop->nitems; i++) {
    if (i > 0) fprintf(fp, ",");
    fprintf(fp, "%f", values[i]);
  }
  fprintf(fp, "\n");
}
void property_coords_load_program(Property *prop, Rendering *rendering) {
  PropertyProgramCache *prop_cache = &prop->programs[rendering->program_cache_idx];
  DEBUG("prop", "%ld[%ld].%s %s (coords) [%d]\n",
        prop->window, prop_cache->program, prop_cache->name_str,
        (prop_cache->location != -1) ? "enabled" : "disabled", prop->nitems);
}
void property_coords_free_program(Property *prop, size_t index) {
}
PropertyTypeHandler property_coords = {&property_coords_init, &property_coords_load, &property_coords_free, &property_coords_to_gl, &property_coords_print, &property_coords_load_program, &property_coords_free_program, NULL, &property_coords_calculate};
