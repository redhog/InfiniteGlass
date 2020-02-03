#include "property_wm_hints_icon.h"
#include "rendering.h"
#include "texture.h"
#include "debug.h"

typedef struct {
  XWMHints wm_hints;
  Texture icon_texture;
  Texture icon_mask_texture;
} WmHintsPropertyData;

typedef struct {
  char *icon_str;
  char *icon_mask_str;
  char *icon_enabled_str;
  char *icon_mask_enabled_str;
  GLint icon_location;
  GLint icon_mask_location;
  GLint icon_enabled_location;
  GLint icon_mask_enabled_location;
} WmHintsPropertyProgramData;

void property_wm_hints_icon_init(PropertyTypeHandler *prop) { prop->type = XInternAtom(display, "WM_HINTS", False); prop->name = AnyPropertyType; }
void property_wm_hints_icon_load(Property *prop) {
  prop->data = malloc(sizeof(WmHintsPropertyData));
  WmHintsPropertyData *data = (WmHintsPropertyData *) prop->data;
  
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
  GL_CHECK_ERROR("icon_update_pixmap2", "%ld", prop->window);
}

void property_wm_hints_icon_free(Property *prop) {
  WmHintsPropertyData *data = (WmHintsPropertyData *) prop->data;
  texture_destroy(&data->icon_texture);
  texture_destroy(&data->icon_mask_texture);
  free(prop->data);
}
void property_wm_hints_icon_to_gl(Property *prop, Rendering *rendering) {
  if (rendering->view->picking) return;
  WmHintsPropertyData *data = (WmHintsPropertyData *) prop->data;
  PropertyProgramCache *prop_cache = &prop->programs[rendering->program_cache_idx];
  WmHintsPropertyProgramData *program_data = (WmHintsPropertyProgramData *) prop_cache->data;
  if (!prop_cache->is_uniform || program_data->icon_location == -1 || program_data->icon_mask_location == -1) return;

  if (data->wm_hints.flags & IconPixmapHint) {
    glUniform1i(program_data->icon_enabled_location, 1);
    glUniform1i(program_data->icon_location, rendering->texture_unit);
    glActiveTexture(GL_TEXTURE0+rendering->texture_unit);
    glBindTexture(GL_TEXTURE_2D, data->icon_texture.texture_id);
    glBindSampler(rendering->texture_unit, 0);
    rendering->texture_unit++;
  }
  if (data->wm_hints.flags & IconMaskHint) {
    glUniform1i(program_data->icon_mask_enabled_location, 1);
    glUniform1i(program_data->icon_mask_location, rendering->texture_unit);
    glActiveTexture(GL_TEXTURE0+rendering->texture_unit);
    glBindTexture(GL_TEXTURE_2D, data->icon_mask_texture.texture_id);
    glBindSampler(rendering->texture_unit, 0);
    rendering->texture_unit++;
  }
}
void property_wm_hints_icon_print(Property *prop, FILE *fp) {
  fprintf(fp, "%ld.%s=<WM_HINTS icon>\n", prop->window, prop->name_str);
}
void property_wm_hints_load_program(Property *prop, Rendering *rendering) {
  PropertyProgramCache *prop_cache = &prop->programs[rendering->program_cache_idx];
  
  prop_cache->data = malloc(sizeof(WmHintsPropertyProgramData));
  WmHintsPropertyProgramData *program_data = (WmHintsPropertyProgramData *) prop_cache->data;

  program_data->icon_str = malloc(strlen(prop->name_str) + strlen("_icon") + 1);
  strcpy(program_data->icon_str, prop->name_str);
  strcpy(program_data->icon_str + strlen(prop->name_str), "_icon");

  program_data->icon_mask_str = malloc(strlen(prop->name_str) + strlen("_icon_mask") + 1);
  strcpy(program_data->icon_mask_str, prop->name_str);
  strcpy(program_data->icon_mask_str + strlen(prop->name_str), "_icon_mask");

  program_data->icon_enabled_str = malloc(strlen(prop->name_str) + strlen("_icon_enabled") + 1);
  strcpy(program_data->icon_enabled_str, prop->name_str);
  strcpy(program_data->icon_enabled_str + strlen(prop->name_str), "_icon_enabled");

  program_data->icon_mask_enabled_str = malloc(strlen(prop->name_str) + strlen("_icon_mask_enabled") + 1);
  strcpy(program_data->icon_mask_enabled_str, prop->name_str);
  strcpy(program_data->icon_mask_enabled_str + strlen(prop->name_str), "_icon_mask_enabled");

  program_data->icon_location = glGetUniformLocation(prop_cache->program, program_data->icon_str);
  program_data->icon_mask_location = glGetUniformLocation(prop_cache->program, program_data->icon_mask_str);
  program_data->icon_enabled_location = glGetUniformLocation(prop_cache->program, program_data->icon_enabled_str);
  program_data->icon_mask_enabled_location = glGetUniformLocation(prop_cache->program, program_data->icon_mask_enabled_str);
  char *icon_status = "";
  char *icon_mask_status = "";
  char *icon_enabled_status = "";
  char *icon_mask_enabled_status = "";
  char *all_status = "";

  if (program_data->icon_location != -1 && program_data->icon_mask_location != -1 && program_data->icon_enabled_location != -1 && program_data->icon_mask_enabled_location != -1) {
    all_status = "enabled";
  } else if (program_data->icon_location == -1 && program_data->icon_mask_location == -1 && program_data->icon_enabled_location == -1 && program_data->icon_mask_enabled_location == -1) {
    all_status = "disabled";
  } else {
    if (program_data->icon_location == -1) icon_status = "icon ";
    if (program_data->icon_mask_location == -1) icon_mask_status = "icon-mask ";
    if (program_data->icon_enabled_location == -1) icon_enabled_status = "icon-enabled ";
    if (program_data->icon_mask_enabled_location == -1) icon_mask_enabled_status = "icon-mask-enabled ";
    all_status = "disabled";
  }

  DEBUG("prop", "%ld[%ld].%s %s%s%s%s%s (icon) [%d]\n",
        prop->window,
        rendering->shader->program, prop->name_str,
        icon_status,
        icon_mask_status,
        icon_enabled_status,
        icon_mask_enabled_status,
        all_status,
        prop->nitems);
}
void property_wm_hints_free_program(Property *prop, size_t index) {
  PropertyProgramCache *prop_cache = &prop->programs[index];
  if (!prop_cache->data) return;
  WmHintsPropertyProgramData *data = (WmHintsPropertyProgramData *) prop_cache->data;  
  free(data->icon_str);
  free(data->icon_mask_str);
  free(data->icon_enabled_str);
  free(data->icon_mask_enabled_str);
  free(data);
}
PropertyTypeHandler property_wm_hints_icon = {&property_wm_hints_icon_init, &property_wm_hints_icon_load, &property_wm_hints_icon_free, &property_wm_hints_icon_to_gl, &property_wm_hints_icon_print, &property_wm_hints_load_program, &property_wm_hints_free_program};

