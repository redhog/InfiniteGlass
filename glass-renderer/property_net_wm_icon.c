#include "property_net_wm_icon.h"
#include "rendering.h"
#include "texture.h"
#include "debug.h"

typedef struct {
  char *enabled_str;
  GLint enabled_location;
  Texture texture;
} NetWmIconPropertyData;

void property_net_wm_icon_init(PropertyTypeHandler *prop) { prop->type = XA_CARDINAL; prop->name = XInternAtom(display, "_NET_WM_ICON", False); }
void property_net_wm_icon_load(Property *prop) {
  prop->data = malloc(sizeof(NetWmIconPropertyData));
  NetWmIconPropertyData *data = (NetWmIconPropertyData *) prop->data;

  data->enabled_str = malloc(strlen(prop->name_str) + strlen("_enabled") + 1);
  strcpy(data->enabled_str, prop->name_str);
  strcpy(data->enabled_str + strlen(prop->name_str), "_enabled");

  texture_initialize(&data->texture);

  texture_from_icon(&data->texture, prop->values.dwords);
  gl_check_error("property_net_wm_icon_load");
}

void property_net_wm_icon_free(Property *prop) {
  NetWmIconPropertyData *data = (NetWmIconPropertyData *) prop->data;
  free(data->enabled_str);
  texture_destroy(&data->texture);
  free(prop->data);
}
void property_net_wm_icon_to_gl(Property *prop, Rendering *rendering) {
  NetWmIconPropertyData *data = (NetWmIconPropertyData *) prop->data;

  if (rendering->view->picking) return;
  
  if (prop->program != rendering->shader->program) {
    prop->program = rendering->shader->program;
    prop->location = glGetUniformLocation(prop->program, prop->name_str);
    data->enabled_location = glGetUniformLocation(prop->program, data->enabled_str);
    char *icon_status = "";
    char *icon_enabled_status = "";
    char *all_status = "";

    if (prop->location != -1 && data->enabled_location != -1) {
      all_status = "enabled";
    } else if (prop->location == -1 && data->enabled_location == -1) {
      all_status = "disabled";
    } else {
      if (prop->location == -1) icon_status = "icon ";
      if (data->enabled_location == -1) icon_enabled_status = "icon-enabled ";
      all_status = "disabled";
    }
    
    DEBUG("prop", "%ld.%s %s%s%s (icon) [%d]\n",
          rendering->shader->program, prop->name_str,
          icon_status,
          icon_enabled_status,
          all_status,
          prop->nitems);
  }
  if (prop->location == -1 || data->enabled_location == -1) return;

  glUniform1i(data->enabled_location, 1);
  glUniform1i(prop->location, rendering->texture_unit);
  glActiveTexture(GL_TEXTURE0+rendering->texture_unit);
  glBindTexture(GL_TEXTURE_2D, data->texture.texture_id);
  glBindSampler(rendering->texture_unit, 0);
  rendering->texture_unit++;
}
void property_net_wm_icon_print(Property *prop, FILE *fp) {
  fprintf(fp, "%s=<_NET_WM_ICON icon>\n", prop->name_str);
}
PropertyTypeHandler property_net_wm_icon = {&property_net_wm_icon_init, &property_net_wm_icon_load, &property_net_wm_icon_free, &property_net_wm_icon_to_gl, &property_net_wm_icon_print};

