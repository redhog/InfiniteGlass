#include <X11/Xutil.h>
#include <GL/gl.h>
#include <GL/glx.h>

typedef struct {
 Window window;
 Pixmap pixmap;
 GLXPixmap glxpixmap;
 GLuint texture_id;

 float space_pos[4][2];
} Item;

Item **items_all;

Item *item_get(Window window);
void item_remove(Item *item);
void item_update_space_pos(Item *item);
void item_update_texture(Item *item);
