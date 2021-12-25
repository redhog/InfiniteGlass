#include "property_net_wm_icon.h"
#include "rendering.h"
#include "texture.h"
#include "debug.h"

typedef struct {
  Texture texture;
} NetWmIconPropertyData;

typedef struct {
  char *enabled_str;
  GLint enabled_location;
} NetWmIconPropertyProgramData;

void property_net_wm_icon_init(PropertyTypeHandler *prop) { prop->type = XA_CARDINAL; prop->name = ATOM("_NET_WM_ICON"); }
void property_net_wm_icon_load(Property *prop) {
  prop->data = malloc(sizeof(NetWmIconPropertyData));
  NetWmIconPropertyData *data = (NetWmIconPropertyData *) prop->data;

  texture_initialize(&data->texture);
  texture_from_icon(&data->texture, prop->values.dwords);
  GL_CHECK_ERROR("property_net_wm_icon_load", "%ld", prop->window);
}

void property_net_wm_icon_free(Property *prop) {
  NetWmIconPropertyData *data = (NetWmIconPropertyData *) prop->data;
  texture_destroy(&data->texture);
  free(prop->data);
}
void property_net_wm_icon_to_gl(Property *prop, Rendering *rendering) {
  if (rendering->view->picking) return;

  NetWmIconPropertyData *data = (NetWmIconPropertyData *) prop->data;
  PropertyProgramCache *prop_cache = &prop->programs[rendering->program_cache_idx];
  NetWmIconPropertyProgramData *program_data = (NetWmIconPropertyProgramData *) prop_cache->data;
  if (!prop_cache->is_uniform || prop_cache->location == -1 || program_data->enabled_location == -1) return;
  
  glUniform1i(program_data->enabled_location, 1);
  glUniform1i(prop_cache->location, rendering->texture_unit);
  glActiveTexture(GL_TEXTURE0+rendering->texture_unit);
  glBindTexture(GL_TEXTURE_2D, data->texture.texture_id);
  glBindSampler(rendering->texture_unit, 0);
  rendering->texture_unit++;
}
void property_net_wm_icon_print(Property *prop, FILE *fp) {
  fprintf(fp, "%ld.%s=<_NET_WM_ICON icon>\n", prop->window, prop->name_str);
}
void property_net_wm_load_program(Property *prop, Rendering *rendering) {
  PropertyProgramCache *prop_cache = &prop->programs[rendering->program_cache_idx];
  prop_cache->data = malloc(sizeof(NetWmIconPropertyProgramData));
  NetWmIconPropertyProgramData *program_data = (NetWmIconPropertyProgramData *) prop_cache->data;

  program_data->enabled_str = malloc(strlen(prop_cache->name_str) + strlen("_enabled") + 1);
  strcpy(program_data->enabled_str, prop_cache->name_str);
  strcpy(program_data->enabled_str + strlen(prop_cache->name_str), "_enabled");

  program_data->enabled_location = glGetUniformLocation(prop_cache->program, program_data->enabled_str);
  char *icon_status = "";
  char *icon_enabled_status = "";
  char *all_status = "";

  if (prop_cache->location != -1 && program_data->enabled_location != -1) {
    all_status = "enabled";
  } else if (prop_cache->location == -1 && program_data->enabled_location == -1) {
    all_status = "disabled";
  } else {
    if (prop_cache->location == -1) icon_status = "icon ";
    if (program_data->enabled_location == -1) icon_enabled_status = "icon-enabled ";
    all_status = "disabled";
  }

  DEBUG("prop", "%ld[%ld].%s %s%s%s (icon) [%d]\n",
        prop->window,
        rendering->shader->program,
        prop->name_str,
        icon_status,
        icon_enabled_status,
        all_status,
        prop->nitems);
}
void property_net_wm_free_program(Property *prop, size_t index) {
  PropertyProgramCache *prop_cache = &prop->programs[index];
  if (!prop_cache->data) return;
  NetWmIconPropertyProgramData *data = (NetWmIconPropertyProgramData *) prop_cache->data;
  if (data->enabled_str) free(data->enabled_str);
  free(data);
  prop_cache->data = NULL;
}
PropertyTypeHandler property_net_wm_icon = {
  .init=&property_net_wm_icon_init,
  .load=&property_net_wm_icon_load,
  .free=&property_net_wm_icon_free,
  .to_gl=&property_net_wm_icon_to_gl,
  .print=&property_net_wm_icon_print,
  .load_program=&property_net_wm_load_program,
  .free_program=&property_net_wm_free_program
};

