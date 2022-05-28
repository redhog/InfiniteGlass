#include "property_views.h"
#include "property_atom.h"
#include "property_float.h"
#include "property_int.h"
#include "view.h"
#include "wm.h"
#include "debug.h"

int property_views_match(Property *prop) {
  if (prop->window != root) return -1;
  if (prop->type == XA_ATOM && prop->name == ATOM("IG_VIEWS")) return 2;
  if (strncmp("IG_VIEW_", prop->name_str, strlen("IG_VIEW_")) == 0) return 2;
  return -1;
}
 
void property_views_load(Property *prop) {
  if (prop->window != root) return; // This shouldn't happen, right?
  if (prop->name == ATOM("IG_VIEWS")) {
    view_free_all(views);
    views = view_load_all();
  } else {
    if (views) {
      for (size_t idx = 0; idx < views->count; idx++) {
        View *v = (View *) views->entries[idx];
        if (prop->name == v->attr_layer) {
          view_load_layer(v);
        } else if (prop->name == v->attr_view) {
          view_load_screen(v);
        }
      }
    }
  }
}

void property_views_free(Property *prop) {
}

void property_views_print(Property *prop, int indent, FILE *fp) {
  if (prop->type == XA_ATOM) {
    property_atom_print(prop, indent, fp);
  } else if (prop->type == XA_FLOAT) {
    property_float_print(prop, indent, fp);
  } else if (prop->type == XA_INTEGER) {
    property_int_print(prop, indent, fp);
  }
}

void property_views_to_gl(Property *prop, Rendering *rendering) {
  if (prop->type == XA_ATOM) {
    property_atom_to_gl(prop, rendering);
  } else if (prop->type == XA_FLOAT) {
    property_float_to_gl(prop, rendering);
  } else if (prop->type == XA_INTEGER) {
    property_int_to_gl(prop, rendering);
  }
}
void property_views_load_program(Property *prop, Rendering *rendering) {
  if (prop->type == XA_ATOM) {
    property_atom_load_program(prop, rendering);
  } else if (prop->type == XA_FLOAT) {
    property_float_load_program(prop, rendering);
  } else if (prop->type == XA_INTEGER) {
    property_int_load_program(prop, rendering);
  }
}
void property_views_free_program(Property *prop, size_t index) {
  if (prop->type == XA_ATOM) {
    property_atom_free_program(prop, index);
  } else if (prop->type == XA_FLOAT) {
    property_float_free_program(prop, index);
  } else if (prop->type == XA_INTEGER) {
    property_int_free_program(prop, index);
  }
}

PropertyTypeHandler property_views = {
  .match=&property_views_match,
  .load=&property_views_load,
  .free=&property_views_free,
  .print=&property_views_print,
  .to_gl=&property_views_to_gl,
  .load_program=&property_views_load_program,
  .free_program=&property_views_free_program
};
