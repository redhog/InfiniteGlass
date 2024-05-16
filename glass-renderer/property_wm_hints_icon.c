#include "property_wm_hints_icon.h"
#include "rendering.h"
#include "texture.h"
#include "debug.h"

typedef struct {
  xcb_icccm_wm_hints_t wm_hints;
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

void property_wm_hints_icon_init(PropertyTypeHandler *prop) { prop->type = ATOM("WM_HINTS"); prop->name = AnyPropertyType; }
void property_wm_hints_icon_load(Property *prop) {
  prop->data = malloc(sizeof(WmHintsPropertyData));
  WmHintsPropertyData *data = (WmHintsPropertyData *) prop->data;
  
  texture_initialize(&data->icon_texture);
  texture_initialize(&data->icon_mask_texture);
  
  xcb_icccm_get_wm_hints_from_reply(&data->wm_hints, prop->property_get_reply);

  if (data->wm_hints.flags & XCB_ICCCM_WM_HINT_ICON_PIXMAP) {
    texture_from_pixmap(&data->icon_texture, data->wm_hints.icon_pixmap);
  }
  if (data->wm_hints.flags & XCB_ICCCM_WM_HINT_ICON_MASK) {
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
  if (rendering->picking) return;
  WmHintsPropertyData *data = (WmHintsPropertyData *) prop->data;
  PropertyProgramCache *prop_cache = &prop->programs[rendering->program_cache_idx];
  WmHintsPropertyProgramData *program_data = (WmHintsPropertyProgramData *) prop_cache->data;
  if (!prop_cache->is_uniform || program_data->icon_location == -1 || program_data->icon_mask_location == -1) return;

  if (data->wm_hints.flags & XCB_ICCCM_WM_HINT_ICON_PIXMAP) {
    glUniform1i(program_data->icon_enabled_location, 1);
    glUniform1i(program_data->icon_location, rendering->texture_unit);
    glActiveTexture(GL_TEXTURE0+rendering->texture_unit);
    glBindTexture(GL_TEXTURE_2D, data->icon_texture.texture_id);
    glBindSampler(rendering->texture_unit, 0);
    rendering->texture_unit++;
  }
  if (data->wm_hints.flags & XCB_ICCCM_WM_HINT_ICON_MASK) {
    glUniform1i(program_data->icon_mask_enabled_location, 1);
    glUniform1i(program_data->icon_mask_location, rendering->texture_unit);
    glActiveTexture(GL_TEXTURE0+rendering->texture_unit);
    glBindTexture(GL_TEXTURE_2D, data->icon_mask_texture.texture_id);
    glBindSampler(rendering->texture_unit, 0);
    rendering->texture_unit++;
  }
}
void property_wm_hints_icon_print(Property *prop, int indent, FILE *fp, int detail) {
  fprintf(fp, "%s%s: !WM_HINTS\n", get_indent(indent), prop->name_str);
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
PropertyTypeHandler property_wm_hints_icon = {
  .init=&property_wm_hints_icon_init,
  .load=&property_wm_hints_icon_load,
  .free=&property_wm_hints_icon_free,
  .to_gl=&property_wm_hints_icon_to_gl,
  .print=&property_wm_hints_icon_print,
  .load_program=&property_wm_hints_load_program,
  .free_program=&property_wm_hints_free_program
};

