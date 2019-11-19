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

void property_net_wm_icon_init(PropertyTypeHandler *prop) { prop->type = XA_CARDINAL; prop->name = XInternAtom(display, "_NET_WM_ICON", False); }
void property_net_wm_icon_load(Property *prop) {
  prop->data = malloc(sizeof(NetWmIconPropertyData));
  NetWmIconPropertyData *data = (NetWmIconPropertyData *) prop->data;

  texture_initialize(&data->texture);

  texture_from_icon(&data->texture, prop->values.dwords);
  gl_check_error("property_net_wm_icon_load");
}

void property_net_wm_icon_free(Property *prop) {
  NetWmIconPropertyData *data = (NetWmIconPropertyData *) prop->data;
  texture_destroy(&data->texture);
  free(prop->data);
  for (size_t i = 0; i < PROGRAM_CACHE_SIZE; i++) {
    PropertyProgramCache *prop_cache = &prop->programs[i];
    if (!prop_cache->data) continue;
    NetWmIconPropertyProgramData *data = (NetWmIconPropertyProgramData *) prop_cache->data;
    free(data->enabled_str);
    free(data);
  }
}
void property_net_wm_icon_to_gl(Property *prop, Rendering *rendering) {
  NetWmIconPropertyData *data = (NetWmIconPropertyData *) prop->data;

  if (rendering->view->picking) return;

  PropertyProgramCache *prop_cache = &prop->programs[rendering->program_cache_idx];
  NetWmIconPropertyProgramData *program_data;
    
  if (prop_cache->program != cache->program) {
    prop_cache->data = malloc(sizeof(NetWmIconPropertyProgramData));
    program_data = (NetWmIconPropertyProgramData *) prop_cache->data;
    
    program_data->enabled_str = malloc(strlen(prop_cache->name_str) + strlen("_enabled") + 1);
    strcpy(program_data->enabled_str, prop_cache->name_str);
    strcpy(program_data->enabled_str + strlen(prop_cache->name_str), "_enabled");

    prop_cache->location = glGetUniformLocation(prop_cache->program, prop_cache->name_str);
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
    
    DEBUG("prop", "%ld.%s %s%s%s (icon) [%d]\n",
          rendering->shader->program, prop->name_str,
          icon_status,
          icon_enabled_status,
          all_status,
          prop->nitems);
  }
  program_data = (NetWmIconPropertyProgramData *) prop_cache->data;
  if (prop_cache->location == -1 || program_data->enabled_location == -1) return;

  glUniform1i(program_data->enabled_location, 1);
  glUniform1i(prop_cache->location, rendering->texture_unit);
  glActiveTexture(GL_TEXTURE0+rendering->texture_unit);
  glBindTexture(GL_TEXTURE_2D, data->texture.texture_id);
  glBindSampler(rendering->texture_unit, 0);
  rendering->texture_unit++;
}
void property_net_wm_icon_print(Property *prop, FILE *fp) {
  fprintf(fp, "%s=<_NET_WM_ICON icon>\n", prop->name_str);
}
PropertyTypeHandler property_net_wm_icon = {&property_net_wm_icon_init, &property_net_wm_icon_load, &property_net_wm_icon_free, &property_net_wm_icon_to_gl, &property_net_wm_icon_print};

