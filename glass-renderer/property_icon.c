#include "property_icon.h"
#include "rendering.h"
#include "texture.h"
#include "debug.h"

typedef struct {
  XWMHints wm_hints;
  char *icon_str;
  char *icon_mask_str;
  char *icon_enabled_str;
  char *icon_mask_enabled_str;
  Texture icon_texture;
  Texture icon_mask_texture;
  GLint icon_location;
  GLint icon_mask_location;
  GLint icon_enabled_location;
  GLint icon_mask_enabled_location;
} WmHintsPropertyData;

void property_icon_init(PropertyTypeHandler *prop) { prop->type = XInternAtom(display, "WM_HINTS", False); }
void property_icon_load(Property *prop) {
  prop->data = malloc(sizeof(WmHintsPropertyData));
  WmHintsPropertyData *data = (WmHintsPropertyData *) prop->data;

  data->icon_str = malloc(strlen(prop->name_str) + strlen("_icon") + 1);
  strcpy(data->icon_str, prop->name_str);
  strcpy(data->icon_str + strlen(prop->name_str), "_icon");

  data->icon_mask_str = malloc(strlen(prop->name_str) + strlen("_icon_mask") + 1);
  strcpy(data->icon_mask_str, prop->name_str);
  strcpy(data->icon_mask_str + strlen(prop->name_str), "_icon_mask");


  data->icon_enabled_str = malloc(strlen(prop->name_str) + strlen("_icon_enabled") + 1);
  strcpy(data->icon_enabled_str, prop->name_str);
  strcpy(data->icon_enabled_str + strlen(prop->name_str), "_icon_enabled");

  data->icon_mask_enabled_str = malloc(strlen(prop->name_str) + strlen("_icon_mask_enabled") + 1);
  strcpy(data->icon_mask_enabled_str, prop->name_str);
  strcpy(data->icon_mask_enabled_str + strlen(prop->name_str), "_icon_mask_enabled");
  
  texture_initialize(&data->icon_texture);
  texture_initialize(&data->icon_mask_texture);

  data->wm_hints.flags = 0;
  XErrorEvent error;
  x_try();
  XWMHints *wm_hints = XGetWMHints(display, prop->window);
  if (wm_hints) {
    data->wm_hints = *wm_hints;
    XFree(wm_hints);
  }
  if (!x_catch(&error)) {
    DEBUG("icon", "Unable to load WM_HINTS: %lu", prop->window);
  }

  if (data->wm_hints.flags & IconPixmapHint) {
    texture_from_pixmap(&data->icon_texture, data->wm_hints.icon_pixmap);
  }
  if (data->wm_hints.flags & IconMaskHint) {
    texture_from_pixmap(&data->icon_mask_texture, data->wm_hints.icon_mask);
  }
  gl_check_error("icon_update_pixmap2");
}

void property_icon_free(Property *prop) {
  WmHintsPropertyData *data = (WmHintsPropertyData *) prop->data;
  free(data->icon_str);
  free(data->icon_mask_str);
  free(data->icon_enabled_str);
  free(data->icon_mask_enabled_str);
  texture_destroy(&data->icon_texture);
  texture_destroy(&data->icon_mask_texture);
  free(prop->data);
}
void property_icon_to_gl(Property *prop, Rendering *rendering) {
  WmHintsPropertyData *data = (WmHintsPropertyData *) prop->data;

  if (rendering->view->picking) return;
  
  if (prop->program != rendering->shader->program) {
    prop->program = rendering->shader->program;
    data->icon_location = glGetUniformLocation(prop->program, data->icon_str);
    data->icon_mask_location = glGetUniformLocation(prop->program, data->icon_mask_str);
    data->icon_enabled_location = glGetUniformLocation(prop->program, data->icon_enabled_str);
    data->icon_mask_enabled_location = glGetUniformLocation(prop->program, data->icon_mask_enabled_str);
    char *icon_status = "";
    char *icon_mask_status = "";
    char *icon_enabled_status = "";
    char *icon_mask_enabled_status = "";
    char *all_status = "";

    if (data->icon_location != -1 && data->icon_mask_location != -1 && data->icon_enabled_location != -1 && data->icon_mask_enabled_location != -1) {
      all_status = "enabled";
    } else if (data->icon_location == -1 && data->icon_mask_location == -1 && data->icon_enabled_location == -1 && data->icon_mask_enabled_location == -1) {
      all_status = "disabled";
    } else {
      if (data->icon_location == -1) icon_status = "icon ";
      if (data->icon_mask_location == -1) icon_mask_status = "icon-mask ";
      if (data->icon_enabled_location == -1) icon_enabled_status = "icon-enabled ";
      if (data->icon_mask_enabled_location == -1) icon_mask_enabled_status = "icon-mask-enabled ";
      all_status = "disabled";
    }
    
    DEBUG("prop", "%ld.%s %s%s%s%s%s (icon) [%d]\n",
          rendering->shader->program, prop->name_str,
          icon_status,
          icon_mask_status,
          icon_enabled_status,
          icon_mask_enabled_status,
          all_status,
          prop->nitems);
  }
  if (data->icon_location == -1 || data->icon_mask_location == -1) return;

  if (data->wm_hints.flags & IconPixmapHint) {
    glUniform1i(data->icon_enabled_location, 1);
    glUniform1i(data->icon_location, rendering->texture_unit);
    glActiveTexture(GL_TEXTURE0+rendering->texture_unit);
    glBindTexture(GL_TEXTURE_2D, data->icon_texture.texture_id);
    glBindSampler(rendering->texture_unit, 0);
    rendering->texture_unit++;
  }
  if (data->wm_hints.flags & IconMaskHint) {
    glUniform1i(data->icon_mask_enabled_location, 1);
    glUniform1i(data->icon_mask_location, rendering->texture_unit);
    glActiveTexture(GL_TEXTURE0+rendering->texture_unit);
    glBindTexture(GL_TEXTURE_2D, data->icon_mask_texture.texture_id);
    glBindSampler(rendering->texture_unit, 0);
    rendering->texture_unit++;
  }
}
void property_icon_print(Property *prop, FILE *fp) {
  fprintf(fp, "%s=<icon>", prop->name_str);
  for (int i = 0; i <prop->nitems; i++) {
    if (i > 0) fprintf(fp, ",");
    fprintf(fp, "%li", prop->values.dwords[i]);
  }
  fprintf(fp, "\n");
}
PropertyTypeHandler property_icon = {&property_icon_init, &property_icon_load, &property_icon_free, &property_icon_to_gl, &property_icon_print};

