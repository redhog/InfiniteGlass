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
    data->ccoords[0] = 0.0;
    data->ccoords[1] = 0.0;
    // Set width/height to in invalid value
    data->ccoords[2] = -1.0;
    data->ccoords[3] = -1.0;
  }
  data->coords = realloc(data->coords, sizeof(float) * prop->nitems);
  for (int i = 0; i < prop->nitems; i++) {
    data->coords[i] = FL(prop->values.dwords[i]);
  }
  if (   data->coords[2] <= 0.0
         || data->coords[3] <= 0.0) {
    DEBUG("invalid_coords", "Invalid coords loaded in %ld: %f, %f, %f, %f\n", prop->window, data->coords[0], data->coords[1], data->coords[2], data->coords[3]);
  } else {
    DEBUG("valid_coords", "%ld.property_coords_load: %f,%f,%f,%f\n", prop->window, data->coords[0], data->coords[1], data->coords[2], data->coords[3]);
  }
}

void property_coords_free(Property *prop) {
  if (prop->data) {
    PropertyCoords *data = (PropertyCoords *) prop->data;
    if (data->coords) free(data->coords);
    free(data);
  }
}
void coord_type_error(Rendering *rendering, Atom type, char *type_name, char *error) {
  char *source = rendering->source_item == root_item ? "root" : (rendering->source_item == rendering->parent_item ? "parent" : "window");
  Window win = rendering->item->window;
  if (rendering->parent_item) {
    Window parent = rendering->parent_item->window;
    ERROR("coord_type",
          "%ld/%ld: Coord type %s[%d] used on %s %s\n",
          parent, win, type_name, type, source, error);
  } else {
    ERROR("coord_type",
          "%ld: Coord type %s[%d] used on %s %s\n",
          win, type, source, error);
  }
}

uint64_t property_coords_calculate(Property *prop, Rendering *rendering) {
  PropertyCoords *data = (PropertyCoords *) prop->data;
  if (!data) return prop->version;
  float *ccoords = data->ccoords;
  float *coords = data->coords;
  ccoords[0] = 0.0;
  ccoords[1] = 0.0;
  // Just an invalid value that's different from the one set for
  // coords in load() so we can trace where this came from
  ccoords[2] = -2.0;
  ccoords[3] = -2.0;

  if (prop->nitems < 4 || prop->nitems % 4 != 0) {
    DEBUG("invalid", "%ld.prop->nitems == %d\n", prop->window, prop->nitems);
    return prop->version;
  };

  // Set these to 0 now, so we can add values below according to IG_COORD_TYPES
  ccoords[2] = 0.0;
  ccoords[3] = 0.0;

  PropertyCoords *parent_data = NULL;
  uint32_t required_version = prop->version;
  if (rendering->parent_item && rendering->parent_item->prop_coords) {
    parent_data = (PropertyCoords *) rendering->parent_item->prop_coords->data;
    required_version = MAX(required_version, rendering->parent_item->prop_coords->version);
  }

  if (required_version == prop->calculated_version) return prop->calculated_version;
  
  int types_nitems = 0;
  uint32_t *types = NULL;
  if (rendering->source_item->prop_coord_types) {
    types_nitems = rendering->source_item->prop_coord_types->nitems;
    types = rendering->source_item->prop_coord_types->values.dwords;
    required_version = MAX(required_version, rendering->source_item->prop_coord_types->version);
  }

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
        coord_type_error(rendering, type, "IG_COORD_PARENT_BASE", "without a parent window");
      }
    } else if (type == ATOM("IG_COORD_PARENT")) {
      if (parent_data) {
        float *pccoords = parent_data->ccoords;
        ccoords[0] += coords[i+0] * pccoords[2];
        ccoords[1] += coords[i+1] * pccoords[3];
        ccoords[2] += coords[i+2] * pccoords[2];
        ccoords[3] += coords[i+3] * pccoords[3];
      } else {
        coord_type_error(rendering, type, "IG_COORD_PARENT", "without a parent window");
      }
    } else if (type == ATOM("IG_COORD_PARENT_X")) {
      if (parent_data) {
        float *pccoords = parent_data->ccoords;
        ccoords[0] += coords[i+0] * pccoords[2];
        ccoords[1] += coords[i+1] * pccoords[2];
        ccoords[2] += coords[i+2] * pccoords[2];
        ccoords[3] += coords[i+3] * pccoords[2];
      } else {
        coord_type_error(rendering, type, "IG_COORD_PARENT_X", "without a parent window");
      }
    } else if (type == ATOM("IG_COORD_PARENT_Y")) {
      if (parent_data) {
        float *pccoords = parent_data->ccoords;
        ccoords[0] += coords[i+0] * pccoords[3];
        ccoords[1] += coords[i+1] * pccoords[3];
        ccoords[2] += coords[i+2] * pccoords[3];
        ccoords[3] += coords[i+3] * pccoords[3];
      } else {
        coord_type_error(rendering, type, "IG_COORD_PARENT_Y", "without a parent window");
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
      coord_type_error(rendering, type, name, "but it is unsupported");
    }
  }
  DEBUG("prop_calc", "%ld.coords <<= %f,%f,%f,%f (%d, %d) [%f,%f,%f,%f]\n",
        prop->window,
        ccoords[0], ccoords[1], ccoords[2], ccoords[3], prop->nitems, types_nitems,
        coords[0], coords[1], coords[2], coords[3]
        );
  return required_version;
}

void property_coords_to_gl(Property *prop, Rendering *rendering) {
  PropertyProgramCache *prop_cache = &prop->programs[rendering->program_cache_idx];
  PropertyCoords *data = (PropertyCoords *) prop->data;

  if (prop_cache->location == -1) { DEBUG("invalid", "%ld.prop_cache->location == -1\n", prop->window); return; };
  if (!prop_cache->is_uniform) { DEBUG("invalid", "%ld.!prop_cache->is_uniform\n", prop->window); return; };
  if (prop_cache->type != GL_FLOAT_VEC4) { DEBUG("invalid", "%ld.prop_cache->type != GL_FLOAT_VEC4\n", prop->window); return; };
  
  if (rendering->print) {
    printf("%s%s: [%f, %f, %f, %f]\n",
           get_indent(rendering->indent), prop->name_str, data->ccoords[0], data->ccoords[1], data->ccoords[2], data->ccoords[3]);
  }
  
  glUniform4f(prop_cache->location, data->ccoords[0], data->ccoords[1], data->ccoords[2], data->ccoords[3]);
  DEBUG("prop", "%ld[%s@%ld].%s (coords) = %f,%f,%f,%f\n",
        prop->window, prop_cache->shader->name_str, prop_cache->program, prop_cache->name_str,
        data->ccoords[0], data->ccoords[1], data->ccoords[2], data->ccoords[3]);
}

void property_coords_print(Property *prop, int indent, FILE *fp, int detail) {
  int limit = (detail == 0 && prop->nitems > 10) ? 10 : prop->nitems;
  PropertyCoords *data = (PropertyCoords *) prop->data;
  fprintf(fp, "%s%s: !COORDS [", get_indent(indent), prop->name_str);
  for (int i = 0; i < limit; i++) {
    if (i > 0) fprintf(fp, ", ");
    fprintf(fp, "%f", data->coords[i]);
  }
  if (limit < prop->nitems) {
    fprintf(fp, "] # Truncated\n");
  } else {
    fprintf(fp, "]\n");
  }
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
