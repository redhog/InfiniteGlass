#include "property_coords.h"
#include "shader.h"
#include "rendering.h"
#include "item.h"
#include "debug.h"
#include <math.h>

#define FL(value) *((float *) &value)
void property_coords_init(PropertyTypeHandler *prop) {
  prop->type = XA_FLOAT;
  prop->name = ATOM("IG_COORDS");
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
  
  int types_nitems = 0;
  uint32_t *types = NULL;
  if (rendering->source_item->prop_coord_types) {
    types_nitems = rendering->source_item->prop_coord_types->nitems;
    types = rendering->source_item->prop_coord_types->values.dwords;
  }

  float *ccoords = data->ccoords;
  float *coords = data->coords;
  ccoords[0] = 0.0;
  ccoords[1] = 0.0;
  ccoords[2] = 0.0;
  ccoords[3] = 0.0;

  for (int i = 0; i < prop->nitems; i += 4) {
    Atom type = ATOM("IG_COORD_DESKTOP");
    if (i / 4 < types_nitems) type = (Atom) types[i / 4];

    if (type == ATOM("IG_COORD_DESKTOP")) {
      ccoords[0] += coords[i+0];
      ccoords[1] += coords[i+1];
      ccoords[2] += coords[i+2];
      ccoords[3] += coords[i+3];
    } else if (type == ATOM("IG_COORD_PARENT_BASE")) {
      if (parent_data) {
        float *pccoords = parent_data->ccoords;
        ccoords[0] += pccoords[0] + coords[i+0] * pccoords[2];
        ccoords[1] += pccoords[1] + coords[i+1] * pccoords[3];
        ccoords[2] += coords[i+2] * pccoords[2];
        ccoords[3] += coords[i+3] * pccoords[3];
      } else {
        ERROR("coord_type", "%d: Coord type IG_COORD_PARENT_BASE[%d] used without a parent window\n", rendering->source_item->window, type);
      }
    } else if (type == ATOM("IG_COORD_PARENT")) {
      if (parent_data) {
        float *pccoords = parent_data->ccoords;
        ccoords[0] += coords[i+0] * pccoords[2];
        ccoords[1] += coords[i+1] * pccoords[3];
        ccoords[2] += coords[i+2] * pccoords[2];
        ccoords[3] += coords[i+3] * pccoords[3];
      } else {
        ERROR("coord_type", "%d: Coord type IG_COORD_PARENT[%d] used without a parent window\n", rendering->source_item->window, type);
      }
    } else if (type == ATOM("IG_COORD_PARENT_X")) {
      if (parent_data) {
        float *pccoords = parent_data->ccoords;
        ccoords[0] += coords[i+0] * pccoords[2];
        ccoords[1] += coords[i+1] * pccoords[2];
        ccoords[2] += coords[i+2] * pccoords[2];
        ccoords[3] += coords[i+3] * pccoords[2];
      } else {
        ERROR("coord_type", "%d: Coord type IG_COORD_PARENT_X[%d] used without a parent window\n", rendering->source_item->window, type);
      }
    } else if (type == ATOM("IG_COORD_PARENT_Y")) {
      if (parent_data) {
        float *pccoords = parent_data->ccoords;
        ccoords[0] += coords[i+0] * pccoords[3];
        ccoords[1] += coords[i+1] * pccoords[3];
        ccoords[2] += coords[i+2] * pccoords[3];
        ccoords[3] += coords[i+3] * pccoords[3];
      } else {
        ERROR("coord_type", "%d: Coord type IG_COORD_PARENT_Y[%d] used without a parent window\n", rendering->source_item->window, type);
      }
    } else if (type == ATOM("IG_COORD_SCREEN_BASE")) {
      float *screen = rendering->view->screen;
      ccoords[0] += screen[0] + coords[i+0] * screen[2];
      ccoords[1] += screen[1] + coords[i+1] * screen[3];
      ccoords[2] += coords[i+2] * screen[2];
      ccoords[3] += coords[i+3] * screen[3];
    } else if (type == ATOM("IG_COORD_SCREEN")) {
      float *screen = rendering->view->screen;
      ccoords[0] += coords[i+0] * screen[2];
      ccoords[1] += coords[i+1] * screen[3];
      ccoords[2] += coords[i+2] * screen[2];
      ccoords[3] += coords[i+3] * screen[3];
    } else if (type == ATOM("IG_COORD_SCREEN_X")) {
      float *screen = rendering->view->screen;
      ccoords[0] += coords[i+0] * screen[2];
      ccoords[1] += coords[i+1] * screen[2];
      ccoords[2] += coords[i+2] * screen[2];
      ccoords[3] += coords[i+3] * screen[2];
    } else if (type == ATOM("IG_COORD_SCREEN_Y")) {
      float *screen = rendering->view->screen;
      ccoords[0] += coords[i+0] * screen[3];
      ccoords[1] += coords[i+1] * screen[3];
      ccoords[2] += coords[i+2] * screen[3];
      ccoords[3] += coords[i+3] * screen[3];
    } else {
      char *name = XGetAtomName(display, type);
      ERROR("coord_type", "%d: Unsupported coord type %s[%d]\n", rendering->source_item->window, name ? name : "<INVALID>", type);
    }
  }
  DEBUG("prop_calc", "%ld[%s@%ld].%s (coords) <<= %f,%f,%f,%f (%d, %d) [%f,%f,%f,%f]\n",
        prop->window, prop_cache->shader->name_str, prop_cache->program, prop_cache->name_str,
        ccoords[0], ccoords[1], ccoords[2], ccoords[3], prop->nitems, types_nitems,
        coords[0], coords[1], coords[2], coords[3]

        );
}

void property_coords_to_gl(Property *prop, Rendering *rendering) {
  PropertyProgramCache *prop_cache = &prop->programs[rendering->program_cache_idx];
  PropertyCoords *data = (PropertyCoords *) prop->data;
  glUniform4f(prop_cache->location, data->ccoords[0], data->ccoords[1], data->ccoords[2], data->ccoords[3]);
  DEBUG("prop", "%ld[%s@%ld].%s (coords) = %f,%f,%f,%f\n",
        prop->window, prop_cache->shader->name_str, prop_cache->program, prop_cache->name_str,
        data->ccoords[0], data->ccoords[1], data->ccoords[2], data->ccoords[3]);
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
  DEBUG("prop", "%ld[%s@%ld].%s %s (coords) [%d]\n",
        prop->window, prop_cache->shader->name_str, prop_cache->program, prop_cache->name_str,
        (prop_cache->location != -1) ? "enabled" : "disabled", prop->nitems);
}
void property_coords_free_program(Property *prop, size_t index) {
}
PropertyTypeHandler property_coords = {
  .init=&property_coords_init,
  .load=&property_coords_load,
  .free=&property_coords_free,
  .to_gl=&property_coords_to_gl,
  .print=&property_coords_print,
  .load_program=&property_coords_load_program,
  .free_program=&property_coords_free_program,
  .calculate=&property_coords_calculate
};
